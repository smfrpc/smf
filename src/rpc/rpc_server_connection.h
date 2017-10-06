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
struct rpc_server_connection_options {
  rpc_server_connection_options(bool _nodelay = false, bool _keepalive = false)
    : nodelay(_nodelay), enable_keepalive(_keepalive) {}
  rpc_server_connection_options(rpc_server_connection_options &&o) noexcept
    : nodelay(std::move(o.nodelay))
    , enable_keepalive(std::move(o.enable_keepalive))
    , keepalive(std::move(o.keepalive)) {}

  const bool nodelay;
  const bool enable_keepalive;

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
  seastar::net::keepalive_params keepalive{seastar::net::tcp_keepalive_params{
    std::chrono::seconds(5),  /*Idle secs, Looks like a good default?*/
    std::chrono::seconds(75), /*Interval secs, Linux default*/
    9 /*Probs count, Linux default*/}};
};

class rpc_server_connection : public rpc_connection {
 public:
  rpc_server_connection(
    seastar::connected_socket                     sock,
    seastar::lw_shared_ptr<rpc_connection_limits> conn_limits,
    seastar::socket_address                       address,
    seastar::lw_shared_ptr<rpc_server_stats>      _stats,
    uint64_t                                      connection_id,
    rpc_server_connection_options                 opts = {})
    : rpc_connection(std::move(sock), conn_limits)
    , remote_address(std::move(address))
    , id(connection_id)
    , stats(_stats)
    , opts_(std::move(opts)) {
    socket.set_nodelay(opts_.nodelay);
    if (opts_.enable_keepalive) {
      socket.set_keepalive(true);
      socket.set_keepalive_parameters(opts_.keepalive);
    }
    stats->active_connections++;
    stats->total_connections++;
  }

  ~rpc_server_connection() { stats->active_connections--; }

  const seastar::socket_address            remote_address;
  const uint64_t                           id;
  seastar::lw_shared_ptr<rpc_server_stats> stats;

  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_server_connection);

 private:
  rpc_server_connection_options opts_;
};
}  // namespace smf
