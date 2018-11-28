// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "smf/rpc_client.h"

#include <memory>
#include <utility>
// seastar
#include <core/execution_stage.hh>
#include <core/reactor.hh>
#include <core/sleep.hh>
#include <net/api.hh>
// smf
#include "smf/log.h"
#include "smf/rpc_recv_context.h"

using namespace std::chrono_literals;

namespace smf {
class invalid_connection_state final : public std::exception {
 public:
  virtual const char *
  what() const noexcept {
    return "tcp connection is invalid. please reconnect()";
  }
};
class remote_connection_error final : public std::exception {
 public:
  virtual const char *
  what() const noexcept {
    return "error with remote connection to server";
  }
};

rpc_client::rpc_client(seastar::ipv4_addr addr) : server_addr(addr) {
  rpc_client_opts opts;
  limits = seastar::make_lw_shared<rpc_connection_limits>(
    opts.memory_avail_for_client, opts.recv_timeout);
}
rpc_client::rpc_client(rpc_client_opts opts) : server_addr(opts.server_addr) {
  limits = seastar::make_lw_shared<rpc_connection_limits>(
    opts.memory_avail_for_client, opts.recv_timeout);
}

rpc_client::rpc_client(rpc_client &&o) noexcept
  : server_addr(o.server_addr), limits(std::move(o.limits)),
    read_counter(o.read_counter), conn(std::move(o.conn)),
    rpc_slots(std::move(o.rpc_slots)), in_filters_(std::move(o.in_filters_)),
    out_filters_(std::move(o.out_filters_)),
    serialize_writes_(std::move(o.serialize_writes_)),
    hist_(std::move(o.hist_)), session_idx_(o.session_idx_) {}

seastar::future<>
rpc_client::stop() {
  if (conn) {
    if (!rpc_slots.empty()) {
      LOG_INFO("Shutting down client with possible sessions open: ",
               rpc_slots.size());
      rpc_slots.clear();
    }
    // proper way of closing connection that is safe
    // of concurrency bugs
    try {
      conn->disable();
      conn->socket.shutdown_input();
      conn->socket.shutdown_output();
    } catch (...) {}
  }
  return seastar::make_ready_future<>();
}
rpc_client::~rpc_client() = default;

void
rpc_client::disable_histogram_metrics() {
  hist_ = nullptr;
}
void
rpc_client::enable_histogram_metrics() {
  if (!hist_) hist_ = histogram::make_lw_shared();
}

seastar::future<stdx::optional<rpc_recv_context>>
rpc_client::raw_send(rpc_envelope e) {
  using opt_recv_t = stdx::optional<rpc_recv_context>;
  if (SMF_UNLIKELY(!conn->is_valid())) {
    return seastar::make_exception_future<opt_recv_t>(
      invalid_connection_state());
  }
  // create the work item
  ++session_idx_;
  ++read_counter;

  DLOG_THROW_IF(rpc_slots.find(session_idx_) != rpc_slots.end(),
                "RPC slot already allocated");
  auto work = seastar::make_lw_shared<work_item>(session_idx_);
  auto measure = is_histogram_enabled() ? hist_->auto_measure() : nullptr;

  rpc_slots.insert({session_idx_, work});
  // critical - without this nothing works
  e.letter.header.mutate_session(session_idx_);

  // apply the first set of outgoing filters, then return promise
  return stage_apply_outgoing_filters(std::move(e))
    .then([this, work](rpc_envelope e) {
      // dispatch the write concurrently!
      dispatch_write(std::move(e));
      return work->pr.get_future();
    })
    .then([this, m = std::move(measure)](opt_recv_t r) mutable {
      if (!r) {
        // nothing to do
        return seastar::make_ready_future<opt_recv_t>(std::move(r));
      }
      // something to do
      return stage_apply_incoming_filters(std::move(r.value()))
        .then([m = std::move(m)](rpc_recv_context ctx) {
          LOG_THROW_IF(ctx.header.compression() !=
                         rpc::compression_flags::compression_flags_none,
                       "client is communicating with a server speaking "
                       "different compression protocols: {}",
                       ctx.header.compression());
          return seastar::make_ready_future<opt_recv_t>(
            opt_recv_t(std::move(ctx)));
        });
    });
}
seastar::future<>
rpc_client::reconnect() {
  fail_outstanding_futures();
  return stop().then([this] {
    conn = nullptr;
    return connect();
  });
}
seastar::future<>
rpc_client::connect() {
  LOG_THROW_IF(!!conn,
               "Client already connected to server: `{}'. connect "
               "called more than once.",
               server_addr);

  auto local = seastar::socket_address(sockaddr_in{AF_INET, INADDR_ANY, {0}});
  return seastar::engine()
    .net()
    .connect(seastar::make_ipv4_address(server_addr), local,
             seastar::transport::TCP)
    .then([this, local](seastar::connected_socket fd) mutable {
      conn =
        seastar::make_lw_shared<rpc_connection>(std::move(fd), local, limits);
      do_reads();
      return seastar::make_ready_future<>();
    });
}
seastar::future<>
rpc_client::dispatch_write(rpc_envelope e) {
  // must protect the conn->ostream
  return seastar::with_semaphore(
    serialize_writes_, 1, [this, ee = std::move(e)]() mutable {
      auto payload_size = ee.size();
      return seastar::with_semaphore(
        conn->limits->resources_available, payload_size,
        [this, e = std::move(ee)]() mutable {
          return rpc_envelope::send(&conn->ostream, std::move(e))
            .handle_exception([this](auto _) {
              LOG_INFO("Handling exception(2): {}", _);
              fail_outstanding_futures();
            });
        });
    });
}

void
rpc_client::fail_outstanding_futures() {
  if (conn && conn->is_valid()) {
    LOG_INFO("Disabling connection to: {}", server_addr);
    conn->disable();
  }
  for (auto &&p : rpc_slots) {
    LOG_INFO("Setting exceptional state for session: {}", p.first);
    p.second->pr.set_exception(remote_connection_error());
  }
}

seastar::future<>
rpc_client::process_one_request() {
  return rpc_recv_context::parse_header(conn.get()).then([this](auto hdr) {
    if (SMF_UNLIKELY(!hdr)) {
      LOG_ERROR("Could not parse response from server. Bad Header");
      conn->set_error("Could not parse header from server");
      fail_outstanding_futures();
      return seastar::make_ready_future<>();
    }
    return rpc_recv_context::parse_payload(conn.get(), std::move(hdr.value()))
      .then([this](stdx::optional<rpc_recv_context> opt) mutable {
        DLOG_THROW_IF(read_counter <= 0, "Internal error. Invalid counter: {}",
                      read_counter);
        if (SMF_UNLIKELY(!opt)) {
          LOG_ERROR("Could not parse response from server. Bad payload");
          conn->set_error("Invalid payload");
          fail_outstanding_futures();
          return seastar::make_ready_future<>();
        }
        uint16_t sess = opt->session();
        auto it = rpc_slots.find(sess);
        if (SMF_UNLIKELY(it == rpc_slots.end())) {
          LOG_ERROR("Cannot find session: {}", sess);
          conn->set_error("Invalid session");
          fail_outstanding_futures();
          return seastar::make_ready_future<>();
        }
        --read_counter;
        it->second->pr.set_value(std::move(opt));
        rpc_slots.erase(it);
        return seastar::make_ready_future<>();
      });
  });
}
seastar::future<>
rpc_client::do_reads() {
  auto self = parent_shared_from_this();
  return seastar::do_until(
           [c = conn] { return !c->is_valid(); },
           [this]() {
             auto timeout = conn->limits->max_body_parsing_duration;
             seastar::timer<> body_timeout;
             body_timeout.set_callback([timeout, this] {
               auto oms =
                 std::chrono::duration_cast<std::chrono::milliseconds>(timeout)
                   .count();
               LOG_ERROR("rpc_client exceeded timeout: {}ms", oms);
               conn->set_error("Connection body parsing exceeded timeout");
               fail_outstanding_futures();
             });
             body_timeout.arm(conn->limits->max_body_parsing_duration);
             return process_one_request().finally(
               [body_timeout = std::move(body_timeout)]() mutable {
                 body_timeout.cancel();
               });
           })
    .finally([self] {})
    .handle_exception([this](auto ep) {
      LOG_INFO("Handling exception: {}", ep);
      fail_outstanding_futures();
    });
}

static thread_local auto incoming_stage = seastar::make_execution_stage(
  "smf::rpc_client::incoming::filter", &rpc_client::apply_incoming_filters);

static thread_local auto outgoing_stage = seastar::make_execution_stage(
  "smf::rpc_client::outgoing::filter", &rpc_client::apply_outgoing_filters);

seastar::future<rpc_recv_context>
rpc_client::apply_incoming_filters(rpc_recv_context ctx) {
  return rpc_filter_apply(&in_filters_, std::move(ctx));
}
seastar::future<rpc_envelope>
rpc_client::apply_outgoing_filters(rpc_envelope e) {
  return rpc_filter_apply(&out_filters_, std::move(e));
}

seastar::future<rpc_recv_context>
rpc_client::stage_apply_incoming_filters(rpc_recv_context ctx) {
  return incoming_stage(this, std::move(ctx));
}
seastar::future<rpc_envelope>
rpc_client::stage_apply_outgoing_filters(rpc_envelope e) {
  return outgoing_stage(this, std::move(e));
}

}  // namespace smf
