#pragma once

// std
#include <core/iostream.hh>
// seastar
#include <core/scattered_message.hh>
// smf
#include "log.h"
#include "rpc/rpc_recv_context.h"

namespace smf {
class rpc_decoder {
  public:
  rpc_decoder(size_t max_size = 0) : max_size_(max_size) {}
  future<> handle(input_stream<char> &in,
                  output_stream<char> &out,
                  distributed<rpc_server_stats> &stats) {
    return parse_rpc_recv_context(in).then([](auto recv_ctx) {
      if(recv_ctx) {
        LOG_INFO("Finished parsing the requeset");
      } else {
        LOG_INFO("Invalid request");
      }
    });
  }

  private:
  const size_t max_size_;
};
}
