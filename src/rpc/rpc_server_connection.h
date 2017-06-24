// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <chrono>
#include <experimental/optional>
// seastar
#include <net/api.hh>
// smf
#include "platform/log.h"
#include "rpc/rpc_connection.h"
#include "rpc/rpc_server_stats.h"

namespace smf {
namespace exp = std::experimental;
inline seastar::net::keepalive_params default_linux_tcp_keepalive() {
  // struct tcp_keepalive_params {
  // TCP_KEEPIDLE (since Linux 2.4)
  //        The time (in seconds) the connection needs to remain idle before
  //        TCP  starts  sending  keepalive  probes,  if  the  socket option
  //        SO_KEEPALIVE has been set on this socket.   This  option  should
  //        not be used in code intended to be portable.
  //
  //     std::chrono::seconds idle;
  //
  // TCP_KEEPINTVL (since Linux 2.4)
  //        The time (in seconds) between individual keepalive probes.  This
  //        option should not be used in code intended to be portable
  //
  //     std::chrono::seconds interval;
  //
  //
  // TCP_KEEPCNT (since Linux 2.4)
  //        The maximum number of keepalive probes TCP  should  send  before
  //        dropping the connection.  This option should not be used in code
  //        intended to be portable.
  //
  //     unsigned count;
  // };
  //// see linux sctp(7) for parameter explanation - not yet supported
  // struct sctp_keepalive_params {
  //     std::chrono::seconds interval; // spp_hbinterval
  //     unsigned count; // spp_pathmaxrt
  // };
  //
  //
  // keepalive_params=boost::variant<tcp_keepalive_params,sctp_keepalive_params>
  //
  return seastar::net::keepalive_params{seastar::net::tcp_keepalive_params{
    std::chrono::seconds(5),  /*Idle secs, Looks like a good default?*/
    std::chrono::seconds(75), /*Interval secs, Linux default*/
    9 /*Probs count, Linux default*/}};
}

struct rpc_server_connection_options {
  rpc_server_connection_options() : nodelay(true), keepalive(exp::nullopt) {}
  bool                                              nodelay;
  exp::optional<seastar::net::tcp_keepalive_params> keepalive;
};
class rpc_server_connection : public rpc_connection {
 public:
  rpc_server_connection(seastar::connected_socket     sock,
                        seastar::socket_address       address,
                        rpc_server_stats *            stats,
                        rpc_server_connection_options opts = {})
    : rpc_connection(std::move(sock))
    , remote_address(address)
    , stats_(THROW_IFNULL(stats))
    , opts_(opts) {
    socket.set_nodelay(opts_.nodelay);
    if (opts_.keepalive) {
      socket.set_keepalive(true);
      socket.set_keepalive_parameters(opts_.keepalive.value());
    }
    stats_->active_connections++;
    stats_->total_connections++;
  }
  ~rpc_server_connection() { stats_->active_connections--; }


  const seastar::socket_address remote_address;

 private:
  rpc_server_stats *            stats_;
  rpc_server_connection_options opts_;
};
}  // namespace smf
