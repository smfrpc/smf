#pragma once
// smf
#include "log.h"
#include "hashing_utils.h"
#include "flatbuffers/rpc_generated.h"
namespace smf {
/// \brief given an output stream and a buf* that points to a serialized
/// smf::fbs::rpc::Payload object buffer in flatbuffers. This class
/// will schedule the sending of the bytes to the destination correctly
/// including compression, crc chekcs, etc
class rpc_send_context {
  public:
  rpc_send_context(onput_stream<char> &out,
                   const uint8_t *buf,
                   uint32_t len,
                   size_t max_request_size = 0)
    : out_(out)
    , buf_(buf)
    , buf_len_(len)
    , max_request_size_(max_request_size) {}

  ///
  ///
  future<> send() {
    static constexpr size_t kRPCHeaderSize = sizeof(fbs::rpc::Header);
    using fbs::rpc::Flags;
    auto crc = crc_32((const char *)buf, len);

    Header hdr(buf_len_, Flags_VERIFYY_PAYLOAD, crc);
    return out_->write(hdr, kRPCHeaderSize)
      .then([this] {
        // TODO(agallego) - compress the headers and so on
        // TODO(agallego) - open zipkin traces - right now we just println
        //
        return out_.write(buf_, buf_len_).then([this] { return out_.flush(); });
      });
  }

  private:
  onput_stream<char> &out_;
  const uint8_t *buf_;
  uint32_t buf_len_;
  const size_t max_request_size_;
};
}
