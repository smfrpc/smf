#include "rpc/rpc_server.h"
#include "log.h"
#include "rpc/rpc_envelope.h"
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
        auto conn =
          make_lw_shared<rpc_server_connection>(std::move(fd), addr, stats_);
        return handle_client_connection(conn);
      });
  })
    .or_terminate(); // end keep_doing
}

future<> rpc_server::stop() { return make_ready_future<>(); }

future<> rpc_server::handle_client_connection(
  lw_shared_ptr<rpc_server_connection> conn) {
  return do_until(
    [conn] { return conn->istream.eof() || conn->has_error(); },
    [this, conn]() mutable {
      // TODO(agallego) -
      // Add connection limits and pass to
      // parse_rpc_recv_context() fn to prevent OOM erros
      return rpc_recv_context::parse(conn->istream)
        .then([this, conn](auto recv_ctx) {
          if(!recv_ctx) {
            conn->set_error("Could not parse the request");
            return make_ready_future<>();
          }
          return this->dispatch_rpc(conn, std::move(recv_ctx.value()))
            .finally([conn] {
              return conn->ostream.flush().finally([conn] {
                if(conn->has_error()) {
                  LOG_ERROR("There was an error with the connection: {}",
                            conn->get_error());
                  // Add metrics!
                  return conn->ostream.close();
                }
                return make_ready_future<>();
                // Add metrics!
              });
            }); // end finally()
        });     //  parse_rpc_recv_context.then()
    });         //  do_until()
}

future<> rpc_server::dispatch_rpc(lw_shared_ptr<rpc_server_connection> conn,
                                  rpc_recv_context &&ctx) {
  if(ctx.request_id() == 0) {
    conn->set_error("Missing request_id. Invalid request");
    return make_ready_future<>();
  }
  if(!routes_.can_handle_request(ctx.request_id(),
                                 ctx.payload->dynamic_headers())) {
    // TODO(agallego) - add metrics
    //
    conn->set_error("Can't find route for request. Invalid");
    return make_ready_future<>();
  }
  // TODO(agallego) - missing incoming filters and outgoing filters
  return routes_.handle(std::move(ctx))
    .then([conn](rpc_envelope e) mutable {
      return smf::rpc_envelope::send(conn->ostream, e.to_temp_buf());
    });
}
} // end namespace
