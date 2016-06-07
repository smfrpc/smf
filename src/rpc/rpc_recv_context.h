#pragma once
// std
#include <experimental/optional>
// seastar
#include <core/iostream.hh>
// smf
#include "log.h"
#include "hashing_utils.h"
#include "flatbuffers/rpc_generated.h"


namespace smf {
namespace exp = std::experimental;
struct rpc_recv_context {
  rpc_recv_context(temporary_buffer<char> hdr, temporary_buffer<char> body)
    : header_buf(std::move(hdr)), body_buf(std::move(body)) {

    assert(header_buf.size() == sizeof(fbs::rpc::Header));

    header = reinterpret_cast<fbs::rpc::Header *>(header_buf.get_write());
    payload = flatbuffers::GetMutableRoot<fbs::rpc::Payload>(
      static_cast<void *>(body_buf.get_write()));
  }

  rpc_recv_context(rpc_recv_context &&o) noexcept
    : header_buf(std::move(o.header_buf)),
      body_buf(std::move(o.body_buf)) {
    assert(header_buf.size() == sizeof(fbs::rpc::Header));

    header = reinterpret_cast<fbs::rpc::Header *>(header_buf.get_write());
    payload = flatbuffers::GetMutableRoot<fbs::rpc::Payload>(
      static_cast<void *>(body_buf.get_write()));
  }
  ~rpc_recv_context() {}


  temporary_buffer<char> header_buf;
  temporary_buffer<char> body_buf;
  /// Notice that this is safe. flatbuffers uses this internally via
  /// `PushBytes()` which is nothing more than
  /// \code
  /// struct foo;
  /// flatbuffers::PushBytes((uint8_t*)foo, sizeof(foo));
  /// \endcode
  /// because the flatbuffers compiler can force only primitive types that
  /// are padded to the largest member size
  /// This is the main reason we are using flatbuffers - no serialization cost
  fbs::rpc::Header *header;
  /// \bfief casts the byte buffer to a pointer of the payload type
  /// This is what the root_type type; is with flatbuffers. if you
  /// look at the generated code it just calls GetMutableRoot<>
  ///
  /// This is the main reason we are using flatbuffers - no serialization cost
  ///
  fbs::rpc::Payload *payload;
};

/// \brief determines if we've correctly parsed the request
/// \return  optional fully parsed request, iff the request is supported
/// i.e: we support the compression algorithm, crc32 etc
///
/// Parses the incoming rpc in 2 steps. First we statically know the
/// size of the header, so we parse sizeof(Header). We with this information
/// we parse the body of the request
///
static future<std::experimental::optional<rpc_recv_context>>
parse_rpc_recv_context(input_stream<char> &in) {
  using namespace std::experimental;
  using ret_type = optional<rpc_recv_context>;

  static constexpr size_t kRPCHeaderSize = sizeof(fbs::rpc::Header);
  LOG_INFO("about to read the size: {}", kRPCHeaderSize);

  return in.read_exactly(kRPCHeaderSize)
    .then([&in](temporary_buffer<char> header) {
      // validate the header
      if(kRPCHeaderSize != header.size()) {
        LOG_ERROR("Invalid header size `{}`. skipping req", header.size());
        return make_ready_future<ret_type>(nullopt);
      }

      LOG_INFO("Got a header buf of size: {}", header.size());
      fbs::rpc::Header *hdr = (fbs::rpc::Header *)header.get_write();

      LOG_INFO("Read the header: size:{}, flags:{}, crc32:{}", hdr->size(),
               hdr->flags(), hdr->crc32());

      // TODO(agallego) - add resource limits like seastar rpc
      // if(max_request_size_ > 0 && hdr->size() > max_request_size_) {
      //   log.error("Request size `{}` exceeded max request size of {}",
      //             hdr->size(), max_request_size_);
      //   return make_ready_future<>();
      // }

      if(hdr->size() == 0) {
        LOG_ERROR("Emty body to parse. skipping");
        return make_ready_future<ret_type>(nullopt);
      }

      // FIXME(agallego) - validate w/ a seastar::gate / semaphore
      //
      return in.read_exactly(hdr->size())
        .then([header_buf =
                 std::move(header)](temporary_buffer<char> body) mutable {
          fbs::rpc::Header *hdr = (fbs::rpc::Header *)header_buf.get_write();
          using namespace fbs::rpc;
          /// HAS TO BE FIRST. This is the last thing that happens on the sender
          /// side. So if the content is compressed, it happens BEFORE it's crc
          /// checked
          if((hdr->flags() & Flags::Flags_VERIFY_PAYLOAD)
             == Flags::Flags_VERIFY_PAYLOAD) {
            uint32_t bodycrc32 = crc_32(body.get(), body.size());
            if(bodycrc32 != hdr->crc32()) {
              LOG_ERROR(
                "Payload checksum `{}` does not match header checksum `{}`",
                bodycrc32, hdr->crc32());
              return make_ready_future<ret_type>(nullopt);
            }
          }

          if((hdr->flags() & Flags::Flags_SNAPPY) == Flags::Flags_SNAPPY) {
            LOG_ERROR("Snappy compression not supported yet");
          }

          return make_ready_future<ret_type>(
            rpc_recv_context(std::move(header_buf), std::move(body)));
        });

    });
}
}
