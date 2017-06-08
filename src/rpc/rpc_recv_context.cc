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
rpc_recv_context::rpc_recv_context(seastar::temporary_buffer<char> &&hdr,
                                   seastar::temporary_buffer<char> &&body)
  : header_buf(std::move(hdr)), body_buf(std::move(body)) {
  assert(header_buf.size() == sizeof(fbs::rpc::Header));
  header = reinterpret_cast<fbs::rpc::Header *>(header_buf.get_write());
  assert(header->size() == body_buf.size());
  payload = flatbuffers::GetMutableRoot<fbs::rpc::Payload>(
    static_cast<void *>(body_buf.get_write()));
}

rpc_recv_context::rpc_recv_context(rpc_recv_context &&o) noexcept
  : header_buf(std::move(o.header_buf)), body_buf(std::move(o.body_buf)) {
  assert(header_buf.size() == sizeof(fbs::rpc::Header));

  header  = reinterpret_cast<fbs::rpc::Header *>(header_buf.get_write());
  payload = flatbuffers::GetMutableRoot<fbs::rpc::Payload>(
    static_cast<void *>(body_buf.get_write()));
}
rpc_recv_context::~rpc_recv_context() {}
uint32_t rpc_recv_context::request_id() const { return payload->meta(); }
uint32_t rpc_recv_context::status() const { return payload->meta(); }


seastar::future<seastar::temporary_buffer<char>> read_payload(
  rpc_connection *conn, rpc_connection_limits *limits, size_t payload_size) {
  if (limits == nullptr) {
    return conn->istream.read_exactly(payload_size);
  } else {
    return limits->wait_for_payload_resources(payload_size)
      .then([payload_size, conn]() {
        return conn->istream.read_exactly(payload_size);
      });
  }
}

constexpr uint32_t max_flatbuffers_size() {
  // 2GB - 1 is the max a flatbuffers::vector<uint8_t> can hold
  //
  // needed so that we have access to the internal typedefs
  using namespace flatbuffers;  // NOLINT
  return static_cast<uint32_t>(FLATBUFFERS_MAX_BUFFER_SIZE);
}

seastar::future<exp::optional<rpc_recv_context>> process_payload(
  rpc_connection *                conn,
  rpc_connection_limits *         limits,
  seastar::temporary_buffer<char> header) {
  using ret_type = exp::optional<rpc_recv_context>;
  auto hdr       = reinterpret_cast<const fbs::rpc::Header *>(header.get());
  return read_payload(conn, limits, hdr->size())
    .then([header_buf =
             std::move(header)](seastar::temporary_buffer<char> body) mutable {
      auto hdr = reinterpret_cast<const fbs::rpc::Header *>(header_buf.get());
      if (hdr->size() != body.size()) {
        LOG_ERROR("Read incorrect number of bytes `{}`, expected header: `{}`",
                  body.size(), *hdr);
        return seastar::make_ready_future<ret_type>();
      }
      if (hdr->size() > max_flatbuffers_size()) {
        LOG_ERROR("Bad payload. Body is >  FLATBUFFERS_MAX_BUFFER_SIZE");
        return seastar::make_ready_future<ret_type>();
      }
      if ((hdr->flags() & fbs::rpc::Flags::Flags_CHECKSUM)
          == fbs::rpc::Flags::Flags_CHECKSUM) {
        const uint32_t xx = xxhash_32(body.get(), body.size());
        if (xx != hdr->checksum()) {
          LOG_ERROR("Payload checksum `{}` does not match header checksum `{}`",
                    xx, hdr->checksum());
          return seastar::make_ready_future<ret_type>();
        }
      }
      rpc_recv_context ctx(std::move(header_buf), std::move(body));
      return seastar::make_ready_future<ret_type>(
        exp::optional<rpc_recv_context>(std::move(ctx)));
    });
}

seastar::future<exp::optional<rpc_recv_context>> rpc_recv_context::parse(
  rpc_connection *conn, rpc_connection_limits *limits) {
  using ret_type = exp::optional<rpc_recv_context>;

  static constexpr size_t kRPCHeaderSize = sizeof(fbs::rpc::Header);

  /// serializes access to the input_stream
  /// without this line you can have interleaved reads on the buffer
  assert(conn->istream_active_parser == 0);
  conn->istream_active_parser++;
  return conn->istream.read_exactly(kRPCHeaderSize)
    .then([conn, limits](seastar::temporary_buffer<char> header) {
      if (kRPCHeaderSize != header.size()) {
        LOG_ERROR_IF(conn->is_valid(),
                     "Invalid header size `{}`, expected `{}`, skipping req",
                     header.size(), kRPCHeaderSize);
        return seastar::make_ready_future<ret_type>();
      }
      auto hdr = reinterpret_cast<const fbs::rpc::Header *>(header.get());

      if (hdr->size() == 0) {
        LOG_ERROR("Emty body to parse. skipping");
        return seastar::make_ready_future<ret_type>();
      }
      return process_payload(conn, limits, std::move(header));
    })
    .finally([conn] { conn->istream_active_parser--; });
}

}  // namespace smf
