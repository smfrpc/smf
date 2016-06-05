#pragma once
// std
#include <experimental/optional>
// smf
#include "log.h"
#include "hashing_utils.h"
#include "flatbuffers/rpc_generated.h"
namespace smf {
using std::experimental::optional;
class rpc_recv_context {
  public:
  rpc_recv_context(input_stream<char> &in, size_t max_request_size)
    : in_(in), max_request_size_(max_request_size) {}


  /// \brief determines if we've correctly parsed the request
  /// \return true iff we fully parsed the request & the request is supported
  /// i.e: we support the compression algorithm, crc32 etc
  ///
  bool is_parsed() const { return parsed_; }

  /// \brief parses the incoming rpc in 2 steps. First we statically know the
  /// size of the header, so we parse sizeof(Header). We with this information
  /// we parse the body of the request
  ///
  future<> parse() {
    static constexpr size_t kRPCHeaderSize = sizeof(fbs::rpc::Header);
    log.info("about to read the size: {}", kRPCHeaderSize);

    return in_.read_exactly(kRPCHeaderSize)
      .then([this](temporary_buffer<char> header) {
        header_buf_ = std::move(header);
        if(kRPCHeaderSize != header_buf_.size()) {
          log.error("Invalid header size. skipping req");
          return make_ready_future<>();
        }

        auto hdr =
          reinterpret_cast<fbs::rpc::Header *>(header_buf_.get_write());

        if(max_request_size_ > 0 && hdr->size() > max_request_size_) {
          log.error("Request size `{}` exceeded max request size of {}",
                    hdr->size(), max_request_size_);
          return make_ready_future<>();
        }

        if(hdr->size() == 0) {
          log.error("Emty body to parse. skipping");
          return make_ready_future<>();
        }

        // FIXME(agallego) - validate w/ a seastar::gate / semaphore
        //
        return in_.read_exactly(hdr->size())
          .then([this](temporary_buffer<char> body) {
            body_buf_ = std::move(body);
            for(uint32_t i = fbs::rpc::Flags::Flags_MIN,
                         j = fbs::rpc::Flags::Flags_MAX,
                         k = fbs::rpc::Flags::Flags_MIN;
                i < j; i = 1 << ++k) {
              switch(i) {
              case fbs::rpc::Flags::Flags_SNAPPY:
                log.error("Snappy compression not supported yet");
                return make_ready_future<>();
                break;
              case fbs::rpc::Flags::Flags_ZLIB:
                log.error("zlip compression not supported yet");
                return make_ready_future<>();
                break;
              case fbs::rpc::Flags::Flags_VERIFY_PAYLOAD:
                uint32_t bodycrc32 = crc_32(body_buf_.get(), body_buf_.size());
                if(bodycrc32 != (*this->header())->crc32()) {
                  log.error(
                    "Payload checksum `{}` does not match header checksum `{}`",
                    bodycrc32, (*this->header())->crc32());
                  // Cannot continue with a bad crc32 payload
                  //
                  return make_ready_future<>();
                }
                break;
              }
            }
            parsed_ = true;
            return make_ready_future<>();
          });

      });
  }

  /// Notice that this is safe. flatbuffers uses this internally via
  /// `PushBytes()` which is nothing more than
  /// \code
  /// struct foo;
  /// flatbuffers::PushBytes((uint8_t*)foo, sizeof(foo));
  /// \endcode
  /// because the flatbuffers compiler can force only primitive types that
  /// are padded to the largest member size
  /// This is the main reason we are using flatbuffers - no serialization cost
  optional<fbs::rpc::Header *> header() {
    if(!parsed_) {
      return std::experimental::nullopt;
    }
    if(!header_) {
      header_ = reinterpret_cast<fbs::rpc::Header *>(header_buf_.get_write());
    }
    return header_;
  }

  /// \bfief casts the byte buffer to a pointer of the payload type
  /// This is what the root_type type; is with flatbuffers. if you
  /// look at the generated code it just calls GetMutableRoot<>
  ///
  /// This is the main reason we are using flatbuffers - no serialization cost
  ///
  optional<fbs::rpc::Payload *> payload() {
    if(!parsed_) {
      return std::experimental::nullopt;
    }
    if(header_->flags() != 0) {
      // no flags supported at the moment. no compression, etc.
      //
      log.error("Header flags are not supported yet");
      return std::experimental::nullopt;
    }
    if(!payload_) {
      payload_ = flatbuffers::GetMutableRoot<fbs::rpc::Payload>(
        static_cast<void *>(body_buf_.get_write()));
    }
    return payload_;
  }

  private:
  input_stream<char> &in_;
  const size_t max_request_size_;
  bool parsed_{false};
  temporary_buffer<char> header_buf_;
  temporary_buffer<char> body_buf_;
  // because of the way that flatbuffers aligns structs, header is simply:
  // hder = (Header*)(void*)header_buf.get()
  fbs::rpc::Header *header_ = nullptr;
  fbs::rpc::Payload *payload_ = nullptr;
};
}
