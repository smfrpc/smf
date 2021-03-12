// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "smf/rpc_client.h"

#include <memory>
#include <optional>
#include <seastar/core/future.hh>
#include <utility>
// seastar
#include <seastar/core/execution_stage.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/sleep.hh>
#include <seastar/core/thread.hh>
#include <seastar/core/with_timeout.hh>
#include <seastar/net/api.hh>
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
  limits_ = seastar::make_lw_shared<rpc_connection_limits>(
    opts.memory_avail_for_client, opts.recv_timeout);
  creds_ = opts.credentials;
  dispatch_gate_ = std::make_unique<seastar::gate>();
  serialize_writes_.ensure_space_for_waiters(1);
}

rpc_client::rpc_client(rpc_client_opts opts) : server_addr(opts.server_addr) {
  limits_ = seastar::make_lw_shared<rpc_connection_limits>(
    opts.memory_avail_for_client, opts.recv_timeout);
  creds_ = opts.credentials;
  dispatch_gate_ = std::make_unique<seastar::gate>();
  serialize_writes_.ensure_space_for_waiters(1);
}

rpc_client::rpc_client(rpc_client &&o) noexcept
  : server_addr(o.server_addr), limits_(std::move(o.limits_)),
    creds_(std::move(o.creds_)), read_counter_(o.read_counter_),
    conn_(std::move(o.conn_)), rpc_slots_(std::move(o.rpc_slots_)),
    in_filters_(std::move(o.in_filters_)),
    out_filters_(std::move(o.out_filters_)),
    dispatch_gate_(std::move(o.dispatch_gate_)),
    serialize_writes_(std::move(o.serialize_writes_)),
    hist_(std::move(o.hist_)), session_idx_(o.session_idx_) {}

seastar::future<>
rpc_client::stop() {
  fail_outstanding_futures();
  // now wait for all the fibers to finish
  return dispatch_gate_->close();
}

rpc_client::~rpc_client() {}

void
rpc_client::disable_histogram_metrics() {
  hist_ = nullptr;
}
void
rpc_client::enable_histogram_metrics() {
  if (!hist_) hist_ = histogram::make_lw_shared();
}

seastar::future<std::optional<rpc_recv_context>>
rpc_client::raw_send(rpc_envelope e) {
  using opt_recv_t = std::optional<rpc_recv_context>;
  if (SMF_UNLIKELY(!is_conn_valid())) {
    return seastar::make_exception_future<opt_recv_t>(
      invalid_connection_state());
  }
  // create the work item
  ++session_idx_;
  ++read_counter_;

  DLOG_THROW_IF(rpc_slots_.find(session_idx_) != rpc_slots_.end(),
                "RPC slot already allocated");
  auto work = seastar::make_lw_shared<work_item>(session_idx_);
  auto measure = is_histogram_enabled() ? hist_->auto_measure() : nullptr;

  rpc_slots_.insert({session_idx_, work});
  // critical - without this nothing works
  e.letter.header.mutate_session(session_idx_);

  // apply the first set of outgoing filters, then return promise
  return stage_outgoing_filters(std::move(e))
    .then([this, work](rpc_envelope e) {
      // dispatch the write concurrently!
      (void)dispatch_write(std::move(e));
      return work->pr.get_future();
    })
    .then([this, m = std::move(measure)](opt_recv_t r) mutable {
      if (!r) {
        // nothing to do
        return seastar::make_ready_future<opt_recv_t>(std::move(r));
      }
      // something to do
      return stage_incoming_filters(std::move(r.value()))
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
    // ensure that gate is closed
    // gates can only be closed but not reopened
    LOG_THROW_IF(!dispatch_gate_->is_closed(),
                 "Dispatch gate is not properly closed. Unrecoverable error.");
    dispatch_gate_ = std::make_unique<seastar::gate>();
    conn_ = nullptr;
    return connect();
  });
}

seastar::future<>
rpc_client::connect() {
  LOG_THROW_IF(is_conn_valid(),
               "Client already connected to server: `{}'. connect "
               "called more than once.",
               server_addr);

  return seastar::async([&] {
    auto socket = seastar::make_lw_shared<seastar::socket>(
      seastar::engine().net().socket());
    auto servaddr = seastar::make_ipv4_address(server_addr);
    auto sockaddr =
      seastar::socket_address(sockaddr_in{AF_INET, INADDR_ANY, {0}});
    auto fd =
      socket->connect(servaddr, sockaddr, seastar::transport::TCP).get();
    if (creds_) {

      fd = seastar::tls::wrap_client(std::move(creds_), std::move(fd),
                                     seastar::sstring{})
             .get();
    }

    conn_ = seastar::make_lw_shared<rpc_connection>(
      std::move(fd), std::move(sockaddr), limits_);

    // dispatch in background
    (void)seastar::with_gate(*dispatch_gate_,
                             [this]() mutable { return do_reads(); });
    seastar::make_ready_future<>().get();
  });
};

seastar::future<>
rpc_client::dispatch_write(rpc_envelope e) {
  // NOTE: The reason for the double gate, is that this future
  // is dispatched in the background
  return seastar::with_gate(
    *dispatch_gate_, [this, e = std::move(e)]() mutable {
      auto payload_size = e.size();
      return seastar::with_semaphore(
        conn_->limits->resources_available, payload_size,
        [this, e = std::move(e)]() mutable {
          return seastar::with_semaphore(
            serialize_writes_, 1, [this, e = std::move(e)]() mutable {
              return rpc_envelope::send(&conn_->ostream, std::move(e))
                .handle_exception([this](auto _) {
                  LOG_INFO("Handling exception(2): {}", _);
                  fail_outstanding_futures();
                });
            });
        });
    });
}

void
rpc_client::fail_outstanding_futures() {
  if (is_conn_valid()) {
    DLOG_TRACE("Disabling connection to: {}", server_addr);
    try {
      // NOTE: This is critical. If we don't shutdown the input
      // the server *might* return data which we will leak since the
      // listening socket is still active. We must unregister from
      // seastar pollers via shutdown_* methods
      conn_->disable();
      conn_->socket.shutdown_input();
      conn_->socket.shutdown_output();
    } catch (...) {}
  }
  while (!rpc_slots_.empty()) {
    auto [session, promise_ptr] = *rpc_slots_.begin();
    LOG_INFO("Setting exceptional state for {} client_id={}", server_addr,
             session);
    promise_ptr->pr.set_exception(remote_connection_error());
    rpc_slots_.erase(rpc_slots_.begin());
  }
}

seastar::future<>
rpc_client::process_one_request() {
  // due to a timeout exception, we make a copy of the conn in the
  // lambda capture param of the lw_shared_ptr
  return rpc_recv_context::parse_header(conn_.get())
    .then([this, conn = conn_](auto hdr) {
      if (SMF_UNLIKELY(!hdr)) {
        conn->set_error("Could not parse header from server");
        fail_outstanding_futures();
        return seastar::make_ready_future<>();
      }
      return rpc_recv_context::parse_payload(conn.get(), std::move(hdr.value()))
        .then([this, conn](std::optional<rpc_recv_context> opt) mutable {
          DLOG_THROW_IF(read_counter_ <= 0,
                        "Internal error. Invalid counter: {}", read_counter_);
          if (SMF_UNLIKELY(!opt)) {
            conn->set_error(
              "Could not parse response from server. Bad payload");
            fail_outstanding_futures();
            return seastar::make_ready_future<>();
          }
          uint16_t sess = opt->session();
          auto it = rpc_slots_.find(sess);
          if (SMF_UNLIKELY(it == rpc_slots_.end())) {
            LOG_ERROR("Cannot find session: {}", sess);
            conn->set_error("Invalid session");
            fail_outstanding_futures();
            return seastar::make_ready_future<>();
          }
          --read_counter_;
          it->second->pr.set_value(std::move(opt));
          rpc_slots_.erase(it);
          return seastar::make_ready_future<>();
        });
    });
}
seastar::future<>
rpc_client::do_reads() {
  auto timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      conn_->limits->max_body_parsing_duration)
                      .count();

  return seastar::do_until(
           [this] { return !is_conn_valid(); },
           [this, timeout_ms]() {
             auto timeout = seastar::timer<>::clock::now() +
                            std::chrono::milliseconds(timeout_ms);
             return seastar::with_timeout(timeout, process_one_request());
           })
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
rpc_client::stage_incoming_filters(rpc_recv_context ctx) {
  return incoming_stage(this, std::move(ctx));
}
seastar::future<rpc_envelope>
rpc_client::stage_outgoing_filters(rpc_envelope e) {
  return outgoing_stage(this, std::move(e));
}

}  // namespace smf
