// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_server.h"
#include "histogram/histogram_seastar_utils.h"
#include "platform/log.h"
#include "rpc/rpc_envelope.h"


namespace smf {

std::ostream &operator<<(std::ostream &o, const smf::rpc_server &s) {
  o << "rpc_server{flags=" << s.flags_ << ", port=" << s.port_
    << ", routes=" << s.routes_ << ", limits=" << *s.limits_.get() << "}";
  return o;
}

rpc_server::rpc_server(distributed<rpc_server_stats> *stats,
                       uint16_t                       port,
                       uint32_t                       flags)
  : stats_(THROW_IFNULL(stats)), port_(port), flags_(flags) {}

rpc_server::~rpc_server() {}

void rpc_server::start() {
  LOG_INFO("Starging server:{}", *this);
  listen_options lo;
  lo.reuse_address = true;
  listener_        = listen(make_ipv4_address({port_}), lo);
  keep_doing([this] {
    return listener_->accept().then(
      [this](connected_socket fd, socket_address addr) mutable {
        auto conn = make_lw_shared<rpc_server_connection>(
          std::move(fd), std::move(addr), stats_);
        // DO NOT return the future. Need to execute in parallel
        handle_client_connection(conn);
      });
  }).handle_exception([](auto ep) {
    // Current and future \ref accept() calls will terminate immediately
    // with an error after listener_->abort_accept().
    // prevent future connections
    try {
      std::rethrow_exception(ep);
    } catch (const std::exception &e) {
      LOG_WARN("Server stopped accepting connections: `{}`", e.what());
    }
  });
}

future<> rpc_server::stop() {
  LOG_WARN("Stopping rpc server: aborting future accept() calls");
  listener_->abort_accept();
  return limits_->reply_gate.close();
}

future<> rpc_server::handle_client_connection(
  lw_shared_ptr<rpc_server_connection> conn) {
  return do_until(
    [conn] { return !conn->is_valid(); },
    [this, conn]() mutable {
      return rpc_recv_context::parse(conn.get(), limits_.get())
        .then([this, conn](auto recv_ctx) mutable {
          if (!recv_ctx) {
            conn->set_error("Could not parse the request");
            return make_ready_future<>();
          }
          auto metric = hist_->auto_measure();
          return smf::rpc_filter_apply(&in_filters_,
                                       std::move(recv_ctx.value()))
            .then([this, conn, metric = std::move(metric)](auto ctx) mutable {
              auto payload_size = ctx.body_buf.size();
              return this->dispatch_rpc(conn, std::move(ctx)).finally([
                this, conn, metric = std::move(metric), payload_size
              ] {
                limits_->release_payload_resources(payload_size);
                return conn->ostream.flush().finally([this, conn] {
                  if (conn->has_error()) {
                    LOG_ERROR("There was an error with the connection: {}",
                              conn->get_error());
                    stats_->local().bad_requests++;
                    stats_->local().active_connections--;
                    LOG_INFO("Closing connection for client: {}",
                             conn->remote_address);
                    return conn->ostream.close();
                  } else {
                    stats_->local().completed_requests++;
                    return make_ready_future<>();
                  }
                });
              });  // end finally()
            })     //  parse_rpc_recv_context.then()
            .handle_exception([this, conn](std::exception_ptr eptr) {
              conn->set_error("Exception parsing request ");
              return conn->ostream.close();
            });
        });
    });
}

future<> rpc_server::dispatch_rpc(lw_shared_ptr<rpc_server_connection> conn,
                                  rpc_recv_context &&                  ctx) {
  if (ctx.request_id() == 0) {
    conn->set_error("Missing request_id. Invalid request");
    return make_ready_future<>();
  }
  if (!routes_.can_handle_request(ctx.request_id(),
                                  ctx.payload->dynamic_headers())) {
    stats_->local().no_route_requests++;
    conn->set_error("Can't find route for request. Invalid");
    return make_ready_future<>();
  }
  stats_->local().in_bytes += ctx.header_buf.size() + ctx.body_buf.size();


  /// the request follow [filters] -> handle -> [filters]
  /// the only way for the handle not to receive the information is if
  /// the filters invalidate the request - they have full mutable access
  /// to it, or they throw an exception if they wish to interrupt the entire
  /// connection
  return seastar::with_gate(
           limits_->reply_gate, [this, ctx = std::move(ctx), conn]() mutable {
             return routes_.handle(std::move(ctx))
               .then([this](rpc_envelope &&e) {
                 return rpc_filter_apply(&out_filters_, std::move(e));
               })
               .then([this, conn](rpc_envelope &&e) {
                 if (e.letter.dtype
                     == rpc_letter_type::rpc_letter_type_payload) {
                   e.letter.mutate_payload_to_binary();
                 }
                 stats_->local().out_bytes +=
                   e.letter.body.size() + rpc_envelope::kHeaderSize;
                 return smf::rpc_envelope::send(&conn->ostream, std::move(e));
               });
           })
    .handle_exception([this, conn](auto ptr) {
      LOG_INFO("Cannot dispatch rpc. Server is shutting down...");
      conn->disable();
      return make_ready_future<>();
    });
}

}  // namespace smf
