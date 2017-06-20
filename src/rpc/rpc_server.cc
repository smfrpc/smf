// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_server.h"

#include <core/prometheus.hh>

#include "histogram/histogram_seastar_utils.h"
#include "platform/log.h"
#include "rpc/rpc_envelope.h"


namespace smf {

std::ostream &operator<<(std::ostream &o, const smf::rpc_server &s) {
  o << "rpc_server{args.flags=" << s.args_.flags
    << ", args.rpc_port=" << s.args_.rpc_port
    << ", args.http_port=" << s.args_.http_port << ", rpc_routes=" << s.routes_
    << ", limits=" << *s.limits_.get() << "}";
  return o;
}

rpc_server::rpc_server(seastar::distributed<rpc_server_stats> *stats,
                       rpc_server_args                         args)
  : stats_(THROW_IFNULL(stats)), args_(args) {}

rpc_server::~rpc_server() {}

void rpc_server::start() {
  LOG_INFO("Starging server:{}", *this);
  if (!(args_.flags & rpc_server_flags_disable_http_server)) {
    LOG_INFO("Starting HTTP admin server on background future");
    admin_ = seastar::make_lw_shared<seastar::http_server>("smf admin server");
    LOG_INFO("HTTP server started, adding prometheus routes");
    seastar::prometheus::config conf;
    conf.metric_help = "smf-broker statistics";
    conf.prefix      = "smf";
    // start on background co-routine
    seastar::prometheus::add_prometheus_routes(*admin_, conf).then([
      http_port = args_.http_port, admin = admin_
    ]() {
      return admin->listen(seastar::ipv4_addr{"127.0.0.1", http_port})
        .handle_exception([](auto ep) {
          LOG_ERROR("Exception on HTTP Admin: {}", ep);
          return seastar::make_exception_future<>(ep);
        });
    });
  }
  LOG_INFO("Starting RPC Server...");
  seastar::listen_options lo;
  lo.reuse_address = true;
  listener_ = seastar::listen(seastar::make_ipv4_address({args_.rpc_port}), lo);
  seastar::keep_doing([this] {
    return listener_->accept().then([this](
      seastar::connected_socket fd, seastar::socket_address addr) mutable {
      auto conn = seastar::make_lw_shared<rpc_server_connection>(
        std::move(fd), std::move(addr), stats_);
      // DO NOT return the future. Need to execute in parallel
      handle_client_connection(conn);
    });
  }).handle_exception([](auto ep) {
    // Current and future \ref accept() calls will terminate immediately
    // with an error after listener_->abort_accept().
    // prevent future connections
    LOG_WARN("Server stopped accepting connections: `{}`", ep);
  });
}

seastar::future<> rpc_server::stop() {
  LOG_WARN("Stopping rpc server: aborting future accept() calls");
  listener_->abort_accept();
  return limits_->reply_gate.close().then([admin = admin_] {
    if (!admin) {
      return seastar::make_ready_future<>();
    }
    return admin->stop().handle_exception([](auto ep) {
      LOG_WARN("Error (ignoring...) shutting down HTTP server: {}", ep);
      return seastar::make_ready_future<>();
    });
  });
}

seastar::future<> rpc_server::handle_client_connection(
  seastar::lw_shared_ptr<rpc_server_connection> conn) {
  return seastar::do_until(
    [conn] { return !conn->is_valid(); },
    [this, conn]() mutable {
      return rpc_recv_context::parse(conn.get(), limits_.get())
        .then([this, conn](auto recv_ctx) mutable {
          if (!recv_ctx) {
            conn->set_error("Could not parse the request");
            return seastar::make_ready_future<>();
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
                    return seastar::make_ready_future<>();
                  }
                });
              });  // end finally()
            })     //  parse_rpc_recv_context.then()
            .handle_exception([this, conn](auto eptr) {
              LOG_ERROR("Caught exception dispatching rpc: {}", eptr);
              conn->set_error("Exception parsing request ");
              return conn->ostream.close();
            });
        });
    });
}

seastar::future<> rpc_server::dispatch_rpc(
  seastar::lw_shared_ptr<rpc_server_connection> conn, rpc_recv_context &&ctx) {
  if (ctx.request_id() == 0) {
    conn->set_error("Missing request_id. Invalid request");
    return seastar::make_ready_future<>();
  }
  if (!routes_.can_handle_request(ctx.request_id(),
                                  ctx.payload->dynamic_headers())) {
    stats_->local().no_route_requests++;
    conn->set_error("Can't find route for request. Invalid");
    return seastar::make_ready_future<>();
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
      return seastar::make_ready_future<>();
    });
}

}  // namespace smf
