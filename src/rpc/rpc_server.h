#pragma once
// seastar
#include <core/distributed.hh>
// smf
#include "log.h"
#include "rpc/rpc_server_stats.h"
#include "rpc/rpc_decoder.h"

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
          log.info("accepted connection");
          auto conn = make_lw_shared<connection>(std::move(fd), addr, stats_);

          // keep open forever until the client shuts down the socket
          // no need to close the connction
          do_until([conn] { return conn->istream().eof(); },
                   [this, conn] {
                     log.info("asking protocol to handle data");
                     return conn->protocol()
                       .handle(conn->istream(), conn->ostream(), stats_)
                       .then([conn] { return conn->ostream().flush(); });
                   })
            .finally([conn] {
              log.info("closing connections");
              return conn->ostream().close().finally([conn] {});
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

  class connection {
    public:
    connection(connected_socket &&sock,
               socket_address address,
               distributed<rpc_server_stats> &stats)
      : socket_(std::move(sock))
      , addr_(address)
      , in_(socket_.input())   // has no alternate ctor
      , out_(socket_.output()) // has no alternate ctor
      , stats_(stats) {
      stats_.local().active_connections++;
      stats_.local().total_connections++;
    }
    ~connection() { stats_.local().active_connections--; }

    input_stream<char> &istream() { return in_; }
    output_stream<char> &ostream() { return out_; }
    rpc_decoder &protocol() { return proto_; }

    private:
    connected_socket socket_;
    socket_address addr_;
    input_stream<char> in_;
    output_stream<char> out_;
    rpc_decoder proto_;
    distributed<rpc_server_stats> &stats_;
  };
};


} /* namespace memcache */
