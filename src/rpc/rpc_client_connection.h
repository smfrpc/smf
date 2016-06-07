#pragma once
// seastar
#include <net/api.hh>
#include <core/iostream.hh>

namespace smf {
class rpc_client_connection {
  public:
  rpc_client_connection(connected_socket &&fd)
    : socket_(std::move(fd)), in_(socket_.input()), out_(socket_.output()) {}
  input_stream<char> &istream() { return in_; };
  output_stream<char> &ostream() { return out_; };

  private:
  connected_socket socket_;
  input_stream<char> in_;
  output_stream<char> out_;
  // TODO(agallego) - add stats
  // rpc_client_stats &stats_;
};
}
