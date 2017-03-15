// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_recv_context.h"
#include "flatbuffers/rpc_generated.h"
#include "hashing/hashing_utils.h"
#include "platform/log.h"

namespace std {
ostream &operator<<(ostream &o, const smf::fbs::rpc::Header &h) {
  o << "fbs::rpc::Header {size:" << h.size() << ", flags:" << h.flags()
    << ", checksum:" << h.checksum() << "}";
  return o;
}
}

namespace smf {
namespace exp = std::experimental;
rpc_recv_context::rpc_recv_context(temporary_buffer<char> &&hdr,
                                   temporary_buffer<char> &&body)
  : header_buf(std::move(hdr)), body_buf(std::move(body)) {
  assert(header_buf.size() == sizeof(fbs::rpc::Header));
  assert(header()->size() == body_buf.size());
}

rpc_recv_context::rpc_recv_context(rpc_recv_context &&o) noexcept
  : header_buf(std::move(o.header_buf)),
    body_buf(std::move(o.body_buf)) {
  assert(header_buf.size() == sizeof(fbs::rpc::Header));
  assert(header()->size() == body_buf.size());
}

rpc_recv_context::~rpc_recv_context() {}
uint32_t rpc_recv_context::request_id() { return payload()->meta(); }
uint32_t rpc_recv_context::status() { return payload()->meta(); }


future<temporary_buffer<char>> read_payload(rpc_connection *       conn,
                                            rpc_connection_limits *limits,
                                            size_t payload_size) {
  if (limits == nullptr) {
    return conn->istream.read_exactly(payload_size);
  } else {
    return limits->wait_for_payload_resources(payload_size)
      .then([payload_size, conn]() {
        return conn->istream.read_exactly(payload_size);
      });
  }
}

future<rpc_recv_context> process_payload(rpc_connection *       conn,
                                         rpc_connection_limits *limits,
                                         temporary_buffer<char> header) {
  auto hdr = reinterpret_cast<const fbs::rpc::Header *>(header.get());
  return read_payload(conn, limits, hdr->size())
    .then([header_buf =
             std::move(header)](temporary_buffer<char> body) mutable {
      auto hdr = reinterpret_cast<const fbs::rpc::Header *>(header_buf.get());
      LOG_THROW_IF(hdr->size() != body.size(),
                   "Read incorrect number of bytes `{}`, expected header: `{}`",
                   body.size(), *hdr);

      if ((hdr->flags() & fbs::rpc::Flags::Flags_CHECKSUM)
          == fbs::rpc::Flags::Flags_CHECKSUM) {
        const uint32_t xx = xxhash_32(body.get(), body.size());

        LOG_THROW_IF(
          xx != hdr->checksum(),
          "Payload checksum `{}` does not match header checksum `{}`", xx,
          hdr->checksum());
      }
      rpc_recv_context ctx(std::move(header_buf), std::move(body));
      return make_ready_future<rpc_recv_context>(std::move(ctx));
    });
}

future<rpc_recv_context> rpc_recv_context::parse(
  rpc_connection *conn, rpc_connection_limits *limits) {
  static constexpr size_t kRPCHeaderSize = sizeof(fbs::rpc::Header);

  /// serializes access to the input_stream
  /// without this line you can have interleaved reads on the buffer
  assert(conn->istream_active_parser == 0);
  conn->istream_active_parser++;
  return conn->istream.read_exactly(kRPCHeaderSize)
    .then([conn, limits](temporary_buffer<char> header) {
      LOG_THROW_IF(kRPCHeaderSize != header.size(),
                   "Invalid header size `{}`, expected `{}`", header.size(),
                   kRPCHeaderSize);

      auto hdr = reinterpret_cast<const fbs::rpc::Header *>(header.get());
      LOG_THROW_IF(hdr->size() == 0, "Emty body to parse. skipping");

      return process_payload(conn, limits, std::move(header));
    })
    .finally([conn] { conn->istream_active_parser--; });
}

}  // namespace smf
