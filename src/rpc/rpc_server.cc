#include "rpc/rpc_server.h"
#include "log.h"
#include "rpc/rpc_envelope.h"
#include "rpc/rpc_envelope_utils.h"
namespace smf {

rpc_server::rpc_server(distributed<rpc_server_stats> &stats, uint16_t port)
  : stats_(stats), port_(port) {}

void rpc_server::start() {
  LOG_INFO("Starging server on: {} ", port_);
  listen_options lo;
  lo.reuse_address = true;
  listener_ = engine().listen(make_ipv4_address({port_}), lo);
  keep_doing([this] {
    return listener_->accept().then(
      [this](connected_socket fd, socket_address addr) mutable {
        LOG_INFO("accepted connection");
        auto conn =
          make_lw_shared<rpc_server_connection>(std::move(fd), addr, stats_);
        return handle_client_connection(conn);
      });
  })
    .or_terminate(); // end keep_doing
}

future<> rpc_server::stop() { return make_ready_future<>(); }

void rpc_server::register_router(std::unique_ptr<rpc_handle_router> r) {
  routers_.push_back(std::move(r));
}


future<> rpc_server::handle_client_connection(
  lw_shared_ptr<rpc_server_connection> conn) {
  LOG_INFO("handle_client_connection");
  return do_until(
    [conn] { return conn->istream().eof() || conn->has_error(); },
    [this, conn]() mutable {
      LOG_INFO("asking protocol to handle data");
      // TODO(agallego) -
      // Add connection limits and pass to
      // parse_rpc_recv_context() fn to prevent OOM erros
      return rpc_recv_context::parse(conn->istream())
        .then([this, conn](auto recv_ctx) {
          if(!recv_ctx) {
            conn->set_error("Could not parse the request");
            return make_ready_future<>();
          }
          return this->dispatch_rpc(conn, std::move(recv_ctx.value()))
            .finally([conn] {
              LOG_INFO("closing connections");
              return conn->ostream().close().finally([conn] {
                if(conn->has_error()) {
                  log.error("There was an error with the connection: {}",
                            conn->get_error());
                }
                // Add metrics!
              });
            }); // end finally()
        });     //  parse_rpc_recv_context.then()
    });         //  do_until()
}

future<> rpc_server::dispatch_rpc(lw_shared_ptr<rpc_server_connection> conn,
                                  rpc_recv_context &&ctx) {
  LOG_INFO("dispatch_rpc");
  if(ctx.request_id() == 0) {
    conn->set_error("Missing request_id. Invalid request");
    return make_ready_future<>();
  }
  auto router = this->find_router(ctx);
  if(router == nullptr) {
    conn->set_error("Can't find router for request. Invalid");
    return make_ready_future<>();
  }
  // TODO(agallego) - missing incoming filters and outgoing filters
  return router->handle(std::move(ctx))
    .then([conn](rpc_envelope e) mutable {
      return rpc_envelope_send(conn->ostream(), std::move(e));
    });
}

rpc_handle_router *rpc_server::find_router(const rpc_recv_context &ctx) {
  for(auto &r : routers_) {
    if(r->can_handle_request(ctx.request_id(),
                             ctx.payload->dynamic_headers())) {
      return r.get();
    }
  }
  return nullptr;
}
} // end namespace
