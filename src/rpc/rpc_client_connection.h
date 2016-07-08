#pragma once
// seastar
#include <net/api.hh>
#include <core/iostream.hh>

namespace smf {
struct rpc_client_connection {
  rpc_client_connection(connected_socket fd)
    : socket(std::move(fd))
    , istream(socket.input())
    , ostream(socket.output()) {}
  connected_socket socket;
  input_stream<char> istream;
  output_stream<char> ostream;
  // TODO(agallego) - add stats
  // rpc_client_stats &stats_;
};
}
