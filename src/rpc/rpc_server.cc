#include "rpc/rpc_server.h"
#include "log.h"
#include "rpc/rpc_envelope.h"
namespace smf {

rpc_server::rpc_server(distributed<rpc_server_stats> &stats,
                       uint16_t port,
                       uint32_t flags)
  : stats_(stats), port_(port), flags_(flags) {}

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

future<> rpc_server::stop() {
  if((flags_ & RPCFLAGS::RPCFLAGS_PRINT_HISTOGRAM_ON_EXIT)
     == RPCFLAGS::RPCFLAGS_PRINT_HISTOGRAM_ON_EXIT) {
    try {
      sstring s = "histogram_shard_" + to_sstring(engine().cpu_id()) + ".txt";
      FILE *fp = fopen(s.c_str(), "w");
      if(fp) {
        hist_->print(fp);
        fclose(fp);
      }
    } catch(...) {
    }
  }
  return make_ready_future<>();
}

future<> rpc_server::handle_client_connection(
  lw_shared_ptr<rpc_server_connection> conn) {
  return do_until(
    [conn] { return conn->istream.eof() || conn->has_error(); },
    [this, conn]() mutable {

      // TODO(agallego) -
      // Add connection limits and pass to
      // parse_rpc_recv_context() fn to prevent OOM erros


      /// FIXME(agallego)
      /// YOU ARE PAYING FOR THE PENALTY TO WAIT ON THE SOCKET. NOT TRUE - proxy
      auto metric = hist_->auto_measure(); // THIS IS INCORRECT
      return rpc_recv_context::parse(conn->istream)
        .then([this, conn, m = std::move(metric)](auto recv_ctx) mutable {
          if(!recv_ctx) {
            m->set_trace(false);
            conn->set_error("Could not parse the request");
            return make_ready_future<>();
          }
          return this->dispatch_rpc(conn, std::move(recv_ctx.value()))
            .finally([this, conn, metric = std::move(m)] {
              metric->set_trace(!conn->has_error());
              return conn->ostream.flush().finally([this, conn] {
                if(conn->has_error()) {
                  LOG_ERROR("There was an error with the connection: {}",
                            conn->get_error());
                  stats_.local().bad_requests++;
                  stats_.local().active_connections--;
                  return conn->ostream.close();
                }
                stats_.local().completed_requests++;
                return make_ready_future<>();
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
    stats_.local().no_route_requests++;
    conn->set_error("Can't find route for request. Invalid");
    return make_ready_future<>();
  }
  stats_.local().in_bytes += ctx.header_buf.size() + ctx.body_buf.size();

  // TODO(agallego) - missing incoming filters and outgoing filters
  return routes_.handle(std::move(ctx))
    .then([this, conn](rpc_envelope e) mutable {
      stats_.local().out_bytes += e.size();
      return smf::rpc_envelope::send(conn->ostream, e.to_temp_buf());
    });
}
} // end namespace
