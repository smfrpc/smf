#pragma once
// seastar
#include <net/api.hh>
#include <core/iostream.hh>

namespace smf {

struct rpc_connection {
  rpc_connection(connected_socket fd)
    : socket(std::move(fd))
    , istream(socket.input())
    , ostream(socket.output()) {}
  connected_socket socket;
  input_stream<char> istream;
  output_stream<char> ostream;


  bool is_valid() { return !istream.eof() && !has_error(); }
  bool has_error() const { return error_.operator bool(); }
  void set_error(const char *e) { error_ = sstring(e); }
  sstring get_error() const { return error_.value(); }
  std::experimental::optional<sstring> error_;

};
}
