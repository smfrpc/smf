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
rpc_client::rpc_client(seastar::ipv4_addr addr) : server_addr(addr) {
  rpc_client_opts opts;
  limits = seastar::make_lw_shared<rpc_connection_limits>(
    opts.basic_req_bloat_size, opts.bloat_mult, opts.memory_avail_for_client,
    opts.recv_timeout);
}
rpc_client::rpc_client(rpc_client_opts opts) : server_addr(opts.server_addr) {
  limits = seastar::make_lw_shared<rpc_connection_limits>(
    opts.basic_req_bloat_size, opts.bloat_mult, opts.memory_avail_for_client,
    opts.recv_timeout);
}

rpc_client::rpc_client(rpc_client &&o) noexcept
  : server_addr(o.server_addr), limits(std::move(o.limits)),
    is_error_state(o.is_error_state), read_counter(o.read_counter),
    conn(std::move(o.conn)), rpc_slots(std::move(o.rpc_slots)),
    in_filters_(std::move(o.in_filters_)),
    out_filters_(std::move(o.out_filters_)),
    serialize_writes_(std::move(o.serialize_writes_)),
    hist_(std::move(o.hist_)), session_idx_(o.session_idx_) {}

seastar::future<>
rpc_client::stop() {
  if (conn) {
    auto fut = seastar::with_timeout(seastar::lowres_clock::now() + 1s,
                                     conn->ostream.close())
                 .then_wrapped([this](auto _) {
                   // after nice shutdow; force it
                   try {
                     conn->socket.shutdown_input();
                     conn->socket.shutdown_output();
                   } catch (...) {}
                 });

    if (!rpc_slots.empty()) {
      LOG_INFO("Shutting down client with possible sessions open: ",
               rpc_slots.size());
      LOG_INFO("Sleeping for 1 sec before forcing shutdown");
      return seastar::sleep(std::chrono::seconds(1))
        .then([this, f = std::move(fut)]() mutable { return std::move(f); });
    }
    // proper way of closing connection that is safe
    // of concurrency bugs
    return std::move(fut);
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
  LOG_THROW_IF(is_error_state, "Cannot send request in error state");
  using opt_recv_t = stdx::optional<rpc_recv_context>;
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
rpc_client::connect() {
  LOG_THROW_IF(conn,
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
    serialize_writes_, 1,
    [ee = std::move(e), self = parent_shared_from_this()]() mutable {
      auto payload_size = ee.size();
      return seastar::with_semaphore(
        self->conn->limits->resources_available,
        self->conn->limits->estimate_request_size(payload_size),
        [self, e = std::move(ee)]() mutable {
          return rpc_envelope::send(&self->conn->ostream, std::move(e))
            .handle_exception([self](auto _) {
              LOG_ERROR("Error sending data: {}", _);
              self->is_error_state = true;
            });
        });
    });
}
static seastar::future<>
process_body_for_self(stdx::optional<rpc_recv_context> opt, auto self) {
  if (!opt) {
    LOG_ERROR("Could not parse response from server. Bad payload");
    self->conn->set_error("Invalid payload");
  } else if (self->read_counter > 0 && opt) {
    --self->read_counter;
    auto &&v = std::move(opt.value());
    auto sess = v.session();
    auto slot = self->rpc_slots[sess];
    self->rpc_slots.erase(sess);
    DLOG_THROW_IF(slot->session != v.session(), "Invalid client sessions");
    using typed_ret = decltype(opt);
    slot->pr.set_value(typed_ret(std::move(v)));
  }

  return seastar::make_ready_future<>();
}
seastar::future<>
rpc_client::do_reads() {
  return seastar::do_until(
           [c = conn] { return !c->is_valid(); },
           [self = parent_shared_from_this()]() mutable {
             return rpc_recv_context::parse_header(self->conn.get())
               .then([self](auto hdr) {
                 if (SMF_UNLIKELY(!hdr)) {
                   if (!self->rpc_slots.empty()) {
                     LOG_ERROR(
                       "Could not parse response from server. Bad Header");
                     self->conn->set_error(
                       "Could not parse header from server");
                   } else {
                     // expected. no outstanding reqs
                     self->conn->disable();
                   }
                   return seastar::make_ready_future<>();
                 }
                 return rpc_recv_context::parse_payload(self->conn.get(),
                                                        std::move(hdr.value()))
                   .then([self](stdx::optional<rpc_recv_context> opt) mutable {
                     return process_body_for_self(std::move(opt), self);
                   });
               });
           })
    .finally([self = parent_shared_from_this()] {})
    .handle_exception([self = parent_shared_from_this()](auto ep) mutable {
      self->is_error_state = true;
      LOG_ERROR_IF(self->read_counter > 0,
                   "Failing all enqueued reads {} for client. Error: {}",
                   self->read_counter, ep);
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
