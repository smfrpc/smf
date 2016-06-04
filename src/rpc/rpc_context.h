#pragma once

#include "flatbuffers/rpc_generated.h"
namespace smf {
struct rpc_context {
  constexpr size_t kHeaderSize = sizeof(fbs::rpc::Header);
  rpc_context(input_stream<char> &in,
              output_stream<char> &out,
              temporary_buffer<char> header)
    : buf(header), in(in), out(out) {}
  input_stream<char> &in;
  output_stream<char> &out;
  temporary_buffer<char> buf;
  fbs::rpc::RPC rpc;
};
}
