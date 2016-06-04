#pragma once
// std
#include <experimental/optional>
// seastar
#include <util/log.hh>
// smf
#include "flatbuffers/rpc_generated.h"
namespace smf {
using std::experimental::optional;
class rpc_context {
  public:
  static constexpr size_t kHeaderSize = sizeof(fbs::rpc::Header);
  rpc_context(input_stream<char> &in,
              output_stream<char> &out,
              temporary_buffer<char> &&header,
              temporary_buffer<char> &&body)
    : in_(in)
    , out_(out)
    , header_buf_(std::move(header))
    , body_buf_(std::move(body)) {}

  optional<fbs::rpc::Header *> header() {
    if(kHeaderSize == header_buf_.size()) {
      if(!header_) {
        header_ = reinterpret_cast<fbs::rpc::Header *>(header_buf_.get_write());
      }
      return header_;
    }
    return std::experimental::nullopt;
  }
  optional<fbs::rpc::Payload *> payload() {
    if(header_ != nullptr) {
      if(header_->flags() != 0) {
        //log.error("Header flags are not supported yet");
        return std::experimental::nullopt;
      }
      if(!payload_) {
        // no flags supported at the moment. no compression, etc.
        //
        payload_ = flatbuffers::GetMutableRoot<fbs::rpc::Payload>(
          static_cast<void *>(body_buf_.get_write()));
      }
      return payload_;
    }
    return std::experimental::nullopt;
  }

  private:
  input_stream<char> &in_;
  output_stream<char> &out_;
  temporary_buffer<char> header_buf_;
  temporary_buffer<char> body_buf_;
  // because of the way that flatbuffers aligns structs, header is simply:
  // hder = (Header*)(void*)header_buf.get()
  fbs::rpc::Header *header_ = nullptr;
  fbs::rpc::Payload *payload_ = nullptr;
};
}
