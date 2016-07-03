#pragma once
// std
#include <experimental/optional>
#include <chrono>
// seastar
#include <net/api.hh>
// smf
#include "rpc/rpc_server_stats.h"

namespace smf {
namespace exp = std::experimental;
inline net::keepalive_params default_linux_tcp_keepalive() {
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
  return net::keepalive_params{net::tcp_keepalive_params{
    std::chrono::seconds(5),  /*Idle secs, Looks like a good default?*/
    std::chrono::seconds(75), /*Interval secs, Linux default*/
    9 /*Probs count, Linux default*/}};
}

struct rpc_server_connection_options {

  rpc_server_connection_options() : nodelay(true), keepalive(exp::nullopt) {}
  bool nodelay;
  exp::optional<net::tcp_keepalive_params> keepalive;
};
class rpc_server_connection {
  public:
  rpc_server_connection(connected_socket &&sock,
                        socket_address address,
                        distributed<rpc_server_stats> &stats,
                        rpc_server_connection_options opts = {})
    : socket_(std::move(sock))
    , addr_(address)
    , in_(socket_.input())   // has no alternate ctor
    , out_(socket_.output()) // has no alternate ctor
    , stats_(stats)
    , opts_(opts) {

    socket_.set_nodelay(opts_.nodelay);
    if(opts_.keepalive) {
      socket_.set_keepalive(true);
      socket_.set_keepalive_parameters(opts_.keepalive.value());
    }
    stats_.local().active_connections++;
    stats_.local().total_connections++;
  }
  ~rpc_server_connection() { stats_.local().active_connections--; }

  input_stream<char> &istream() { return in_; }
  output_stream<char> &ostream() { return out_; }
  bool has_error() const { return has_error_; }
  void set_error(const char *e) {
    has_error_ = true;
    error_ = sstring(e);
  }
  sstring get_error() const { return error_; }

  private:
  connected_socket socket_;
  socket_address addr_;
  input_stream<char> in_;
  output_stream<char> out_;
  distributed<rpc_server_stats> &stats_;
  rpc_server_connection_options opts_;
  bool has_error_{false};
  sstring error_;
};
}
