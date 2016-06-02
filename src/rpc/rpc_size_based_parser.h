#pragma once

#include<core/iostream.hh>
#include<core/scattered_message.hh>

namespace smf {
// expects the first 4 bytes to be a little endian
// 32 bit integer.
class rpc_size_based_parser {
  public:
  rpc_size_based_parser(size_t max_size = 0) : max_size_(max_size) {}
  void parse() {}
  future<> handle(input_stream<char> &in, output_stream<char> &out) {

    return make_ready_future<>();
  }
  private:
  scattered_message<char> buf_{};
  size_t max_size_;
};
}
