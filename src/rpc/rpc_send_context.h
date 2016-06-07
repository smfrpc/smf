#pragma once
// seastar
#include <core/iostream.hh>
// smf
#include "log.h"
#include "rpc/rpc_request.h"
namespace smf {
/// \brief given an output stream and a buf* that points to a serialized
/// smf::fbs::rpc::Payload object buffer in flatbuffers. This class
/// will schedule the sending of the bytes to the destination correctly
/// including compression, crc chekcs, etc
future<> rpc_send_context(output_stream<char> &out, rpc_request &&req) {
  static constexpr size_t kRPCHeaderSize = sizeof(fbs::rpc::Header);
  LOG_INFO("Header size of {}", kRPCHeaderSize);
  // TODO(agallego) - add header such as session id, etc to the rpc
  // request headers
  LOG_INFO("finishing buffer");
  req.finish();
  fbs::rpc::Header hdr = req.header();
  LOG_INFO("writing header");
  return out.write((char *)&hdr, kRPCHeaderSize)
    .then([&out, r = std::move(req)]() mutable {
      LOG_INFO("Writing body of size: {}", r.length());
      return out.write((char *)r.buffer(), r.length())
        .then([&out] {
          LOG_INFO("Flushing request");
          return out.flush();
        });
    });
}
}
