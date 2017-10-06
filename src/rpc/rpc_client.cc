// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_client.h"
#include <memory>
#include <utility>
// seastar
#include <core/execution_stage.hh>
#include <core/reactor.hh>
#include <net/api.hh>
// smf
#include "platform/log.h"

namespace smf {
rpc_client::rpc_client(seastar::ipv4_addr addr) : server_addr(std::move(addr)) {
  rpc_client_opts opts;
  limits = seastar::make_lw_shared<rpc_connection_limits>(
    opts.basic_req_bloat_size, opts.bloat_mult, opts.memory_avail_for_client,
    opts.recv_timeout);
}
rpc_client::rpc_client(rpc_client_opts opts)
  : server_addr(std::move(opts.server_addr)) {
  limits = seastar::make_lw_shared<rpc_connection_limits>(
    opts.basic_req_bloat_size, opts.bloat_mult, opts.memory_avail_for_client,
    opts.recv_timeout);
}

rpc_client::rpc_client(rpc_client &&o) noexcept
  : server_addr(std::move(o.server_addr))
  , limits(std::move(o.limits))
  , is_error_state(std::move(o.is_error_state))
  , read_counter(std::move(o.read_counter))
  , conn(std::move(o.conn))
  , rpc_slots(std::move(o.rpc_slots))
  , in_filters_(std::move(o.in_filters_))
  , out_filters_(std::move(o.out_filters_))
  , serialize_writes_(std::move(o.serialize_writes_))
  , hist_(std::move(o.hist_))
  , session_idx_(std::move(o.session_idx_)) {}


seastar::future<> rpc_client::stop() {
  if (conn) {
    auto c = conn;
    c->socket.shutdown_input();
    return c->istream.close().then_wrapped([c](auto _i) {
      return c->ostream.flush().then_wrapped([c](auto _j) {
        return c->ostream.close()
          .then_wrapped([](auto _k) { return seastar::make_ready_future<>(); })
          .finally([c] { c->socket.shutdown_output(); });
      });
    });
  }
  return seastar::make_ready_future<>();
}
rpc_client::~rpc_client() {}

void rpc_client::disable_histogram_metrics() { hist_ = nullptr; }
void rpc_client::enable_histogram_metrics() {
  if (!hist_) hist_ = histogram::make_lw_shared();
}

seastar::future<> rpc_client::connect() {
  LOG_THROW_IF(conn, "Client already connected to server: `{}'. connect "
                     "called more than once.",
               server_addr);

  auto local = seastar::socket_address(sockaddr_in{AF_INET, INADDR_ANY, {0}});
  return seastar::engine()
    .net()
    .connect(seastar::make_ipv4_address(server_addr), local,
             seastar::transport::TCP)
    .then([this](seastar::connected_socket fd) mutable {
      conn = seastar::make_lw_shared<rpc_connection>(std::move(fd), limits);
      do_reads();
      return seastar::make_ready_future<>();
    });
}
seastar::future<> rpc_client::dispatch_write(rpc_envelope e) {
  // must protect the conn->ostream
  return serialize_writes_.wait(1).then(
    [ee = std::move(e), self = parent_shared_from_this(), this]() mutable {
      auto payload_size = ee.size();
      return self->conn->limits->wait_for_payload_resources(payload_size)
        .then([self, e = std::move(ee)]() mutable {
          return smf::rpc_envelope::send(&self->conn->ostream, std::move(e))
            .handle_exception([self](auto _) {
              LOG_ERROR("Error sending data: {}", _);
              self->is_error_state = true;
            });
        })
        .finally([this, self, payload_size] {
          self->conn->limits->release_payload_resources(payload_size);
          serialize_writes_.signal(1);
        });
    });
}

seastar::future<> rpc_client::do_reads() {
  return seastar::do_until(
           [c = conn] { return !c->is_valid(); },
           [self = parent_shared_from_this()]() mutable {
             return smf::rpc_recv_context::parse(self->conn.get())
               .then([self](exp::optional<rpc_recv_context> &&opt) mutable {
                 if (self->read_counter > 0) {
                   if (opt) {
                     --self->read_counter;
                     auto &&v    = std::move(opt.value());
                     auto   sess = v.session();
                     auto   slot = self->rpc_slots[sess];
                     self->rpc_slots.erase(sess);
                     DLOG_THROW_IF(slot->session != v.session(),
                                   "Invalid client sessions");
                     using typed_ret = decltype(opt);
                     slot->pr.set_value(typed_ret(std::move(v)));
                   } else {
                     LOG_ERROR("Could not parse response from server");
                   }
                 }
               });
           })
    .finally([self = parent_shared_from_this()]{})
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


seastar::future<rpc_recv_context> rpc_client::apply_incoming_filters(
  rpc_recv_context ctx) {
  return rpc_filter_apply(&in_filters_, std::move(ctx));
}
seastar::future<rpc_envelope> rpc_client::apply_outgoing_filters(
  rpc_envelope e) {
  return rpc_filter_apply(&out_filters_, std::move(e));
}

seastar::future<rpc_recv_context> rpc_client::stage_apply_incoming_filters(
  rpc_recv_context ctx) {
  return incoming_stage(this, std::move(ctx));
}
seastar::future<rpc_envelope> rpc_client::stage_apply_outgoing_filters(
  rpc_envelope e) {
  return outgoing_stage(this, std::move(e));
}

}  // namespace smf
