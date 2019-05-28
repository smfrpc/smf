// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <chrono>
// seastar
#include <seastar/net/api.hh>
// smf
#include "smf/log.h"
#include "smf/rpc_connection.h"
#include "smf/rpc_server_stats.h"
#include "smf/std-compat.h"
namespace smf {
struct rpc_server_connection_options {
  explicit rpc_server_connection_options(bool _nodelay = false,
                                         bool _keepalive = false)
    : nodelay(_nodelay), enable_keepalive(_keepalive) {}
  rpc_server_connection_options(rpc_server_connection_options &&o) noexcept
    : nodelay(o.nodelay), enable_keepalive(o.enable_keepalive),
      keepalive(std::move(o.keepalive)) {}

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

class rpc_server_connection final {
 public:
  rpc_server_connection(
    seastar::connected_socket sock,
    seastar::lw_shared_ptr<rpc_connection_limits> conn_limits,
    seastar::socket_address address,
    seastar::lw_shared_ptr<rpc_server_stats> _stats, uint64_t connection_id,
    rpc_server_connection_options opts = rpc_server_connection_options(true,
                                                                       true))
    : conn(std::move(sock), address, conn_limits), id(connection_id),
      stats(_stats), opts_(std::move(opts)) {
    // TODO(agallego) - maybe set to true?
    conn.socket.set_nodelay(opts_.nodelay);
    if (opts_.enable_keepalive) {
      conn.socket.set_keepalive(true);
      conn.socket.set_keepalive_parameters(opts_.keepalive);
    }
    stats->active_connections++;
    stats->total_connections++;
  }

  ~rpc_server_connection() { stats->active_connections--; }

  SMF_ALWAYS_INLINE void
  set_error(seastar::sstring e) {
    conn.set_error(std::move(e));
  }
  SMF_ALWAYS_INLINE bool
  has_error() const {
    return conn.has_error();
  }
  SMF_ALWAYS_INLINE seastar::sstring
  get_error() const {
    return conn.get_error();
  }

  SMF_ALWAYS_INLINE bool
  is_valid() {
    return conn.is_valid();
  }
  SMF_ALWAYS_INLINE seastar::lw_shared_ptr<rpc_connection_limits>
  limits() {
    return conn.limits;
  }

  rpc_connection conn;
  const uint64_t id;
  seastar::lw_shared_ptr<rpc_server_stats> stats;
  seastar::semaphore serialize_writes{1};

  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_server_connection);

 private:
  rpc_server_connection_options opts_;
};
}  // namespace smf
