#pragma once

// std
#include <core/iostream.hh>
// seastar
#include <core/scattered_message.hh>
// smf
#include "rpc/rpc_context.h"

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
        auto hdr = reinterpret_cast<fbs::rpc::Header *>(header.get_write());
        // FIXME(agallego) - validate w/ a seastar::gate / semaphore
        //
        return in.read_exactly(hdr->size())
          .then([header = std::move(header), &in, &out, &stats](
            temporary_buffer<char> body) {

            out.write("hello", 5);
            // rpc_context ctx(in, out, std::move(header));
            return make_ready_future<>();
          });

      });
  }

  private:
  const size_t max_size_;
};
}
