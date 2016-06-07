#pragma once
// seastar
#include <core/distributed.hh>
// smf
#include "log.h"
#include "rpc/rpc_server_connection.h"


namespace smf {

class rpc_server {
  public:
  rpc_server(distributed<rpc_server_stats> &stats, uint16_t port)
    : stats_(stats), port_(port) {}

  void start() {
    listen_options lo;
    lo.reuse_address = true;
    listener_ = engine().listen(make_ipv4_address({port_}), lo);
    keep_doing([this] {
      return listener_->accept().then(
        [this](connected_socket fd, socket_address addr) mutable {
          LOG_INFO("accepted connection");
          auto conn =
            make_lw_shared<rpc_server_connection>(std::move(fd), addr, stats_);
          return do_until(
                   [conn] { return conn->istream().eof() || conn->error(); },
                   [this, conn] {
                     LOG_INFO("asking protocol to handle data");
                     return conn->protocol()
                       .handle(conn->istream(), conn->ostream(), stats_)
                       .then([conn] { return conn->ostream().flush(); });
                   })
            .finally([conn] {
              LOG_INFO("closing connections");
              return conn->ostream().close().finally([conn] {
                if(conn->error()) {
                  log.error("There was an error with the connection");
                }
                // Add metrics!
              });
            });
        });
    })
      .or_terminate();
  }

  future<> stop() { return make_ready_future<>(); }

  private:
  lw_shared_ptr<server_socket> listener_;
  distributed<rpc_server_stats> &stats_;
  uint16_t port_;
};

} /* namespace memcache */
