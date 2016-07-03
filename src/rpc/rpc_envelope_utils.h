#pragma once
// seastar
#include <core/future.hh>
#include <core/iostream.hh>
//smf
#include "log.h"
#include "rpc/rpc_envelope.h"
namespace smf {
inline future<> rpc_envelope_send(output_stream<char> &out,
                                  rpc_envelope &&req) {
  static constexpr size_t kRPCHeaderSize = sizeof(fbs::rpc::Header);
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
