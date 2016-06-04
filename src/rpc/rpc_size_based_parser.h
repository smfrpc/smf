#pragma once

#include <core/iostream.hh>
#include <core/scattered_message.hh>

namespace smf {
// expects the first 4 bytes to be a little endian
// 32 bit integer.
class rpc_size_based_parser {
  public:
  rpc_size_based_parser(size_t max_size = 0) : max_size_(max_size) {}
  future<> handle(input_stream<char> &in,
                  output_stream<char> &out,
                  distributed<rpc_stats> &stats) {
    return in.read_exactly(rpc_context::kHeaderSize)
      .then([&in, &out, &stats](temporary_buffer<char> header) {
        // TODO(agallego) - do some validation, like a very large size reuest
        // needs to return a future
        rpc_context ctx(in, out, std::move(header));

      });
  }

  private:
  scattered_message<char> buf_{};
  size_t max_size_;
};
}
