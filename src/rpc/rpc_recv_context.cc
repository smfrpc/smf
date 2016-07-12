#include "rpc/rpc_recv_context.h"
#include "log.h"
#include "hashing_utils.h"
#include "flatbuffers/rpc_generated.h"


namespace smf {
using namespace std::experimental;
rpc_recv_context::rpc_recv_context(temporary_buffer<char> hdr,
                                   temporary_buffer<char> body)
  : header_buf(std::move(hdr)), body_buf(std::move(body)) {

  assert(header_buf.size() == sizeof(fbs::rpc::Header));

  header = reinterpret_cast<fbs::rpc::Header *>(header_buf.get_write());
  payload = flatbuffers::GetMutableRoot<fbs::rpc::Payload>(
    static_cast<void *>(body_buf.get_write()));
}

rpc_recv_context::rpc_recv_context(rpc_recv_context &&o) noexcept
  : header_buf(std::move(o.header_buf)),
    body_buf(std::move(o.body_buf)) {
  assert(header_buf.size() == sizeof(fbs::rpc::Header));

  header = reinterpret_cast<fbs::rpc::Header *>(header_buf.get_write());
  payload = flatbuffers::GetMutableRoot<fbs::rpc::Payload>(
    static_cast<void *>(body_buf.get_write()));
}
rpc_recv_context::~rpc_recv_context() {}
uint32_t rpc_recv_context::request_id() const { return payload->meta(); }
uint32_t rpc_recv_context::status() const { return payload->meta(); }

future<optional<rpc_recv_context>>
rpc_recv_context::parse(input_stream<char> &in) {
  using ret_type = optional<rpc_recv_context>;

  static constexpr size_t kRPCHeaderSize = sizeof(fbs::rpc::Header);
  return in.read_exactly(kRPCHeaderSize)
    .then([&in](temporary_buffer<char> header) {
      // validate the header
      if(kRPCHeaderSize != header.size()) {
        if(in.eof()) {
          LOG_ERROR("Input stream failed to read. input_stream::eof");
        } else {
          LOG_ERROR("Invalid header size `{}`. skipping req", header.size());
        }
        return make_ready_future<ret_type>(nullopt);
      }
      fbs::rpc::Header *hdr = (fbs::rpc::Header *)header.get_write();

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

} // end namespace
