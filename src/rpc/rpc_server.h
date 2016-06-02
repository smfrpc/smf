#pragma once
#include <core/distributed.hh>
#include <util/log.hh>
// smf
#include "rpc/rpc_stats.h"
#include "rpc/rpc_size_based_parser.h"

namespace smf {
class rpc_server {
  public:
  rpc_server(distributed<rpc_stats> &stats, uint16_t port)
    : stats_(stats), port_(port) {}

  void start() {
    listen_options lo;
    lo.reuse_address = true;
    listener_ = engine().listen(make_ipv4_address({port_}), lo);
    keep_doing([this] {
      return listener_->accept().then(
        [this](connected_socket fd, socket_address addr) mutable {
          auto conn = make_lw_shared<connection>(std::move(fd), addr, stats_);

          // keep open forever until the client shuts down the socket
          // no need to close the connction
          do_until([conn] { return conn->in.eof(); },
                   [this, conn] {
                     return conn->proto.handle(conn->in, conn->out)
                       .then([conn] { return conn->out.flush(); });
                   })
            .finally([conn] { return conn->out.close().finally([conn] {}); });
        });
    })
      .or_terminate();
  }

  future<> stop() { return make_ready_future<>(); }

  private:
  lw_shared_ptr<server_socket> listener_;
  distributed<rpc_stats> &stats_;
  uint16_t port_;
  struct connection {
    connected_socket socket;
    socket_address addr;
    input_stream<char> in;
    output_stream<char> out;
    rpc_size_based_parser proto;
    distributed<rpc_stats> &stats;
    connection(connected_socket &&socket,
               socket_address addr,
               distributed<rpc_stats> &stats)
      : socket(std::move(socket))
      , addr(addr)
      , in(socket.input())   // has no alternate ctor
      , out(socket.output()) // has no alternate ctor
      , stats(stats) {
      stats.local().active_connections++;
      stats.local().total_connections++;
    }
    ~connection() { stats.local().active_connections--; }
  };
};


} /* namespace memcache */
