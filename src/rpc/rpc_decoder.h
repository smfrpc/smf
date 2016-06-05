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
    auto recv_ctx = std::make_unique<rpc_recv_context>(in, max_size_);
    return recv_ctx->parse().then([recv_ctx = std::move(recv_ctx)] {
      if(recv_ctx->is_parsed()) {
        log.info("Finished parsing the requeset");
      } else {
        log.error("Invalid request");
      }
    });
  }

  private:
  const size_t max_size_;
};
}
