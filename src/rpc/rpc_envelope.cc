#include "rpc/rpc_envelope.h"
#include <cstring>
// smf
#include "log.h"
#include "rpc/rpc_utils.h"

namespace smf {
future<> rpc_envelope::send(output_stream<char> &out, rpc_envelope e) {
  e.finish();
  temporary_buffer<char> header(kHeaderSize);
  // copy the remaining header bytes
  std::copy((char *)&e.header, (char *)&e.header + kHeaderSize,
            header.get_write());
  // needs to be moved so we can do zero copy output buffer
  return out.write(std::move(header))
    .then([&out, e = std::move(e)]() mutable {
      if((e.header.flags() & fbs::rpc::Flags::Flags_ZSTD)
         == fbs::rpc::Flags::Flags_ZSTD) {
        auto p = std::move(e.get_compressed_payload());
        return out.write(std::move(p));
      } else {
        return out.write(reinterpret_cast<const char *>(e.payload()),
                         e.payload_size());
      }
    })
    .then([&out] { return out.flush(); });
}

rpc_envelope::rpc_envelope(const flatbuffers::FlatBufferBuilder &fbb) {
  init(fbb.GetBufferPointer(), fbb.GetSize());
}
rpc_envelope::rpc_envelope(const char *buf_to_copy) {
  init((uint8_t *)buf_to_copy,
       buf_to_copy == nullptr ? 0 : std::strlen(buf_to_copy));
}
rpc_envelope::rpc_envelope(const char *buf_to_copy, size_t len) {
  static_assert(sizeof(char) == sizeof(uint8_t), "char is not 8 bits");
  init((uint8_t *)buf_to_copy, len);
}
rpc_envelope::rpc_envelope(const uint8_t *buf_to_copy, size_t len) {
  init(buf_to_copy, len);
}
rpc_envelope::rpc_envelope(const sstring &buf_to_copy) {
  init((uint8_t *)buf_to_copy.data(), buf_to_copy.size());
}

rpc_envelope::rpc_envelope(rpc_envelope &&o) noexcept
  : fbb_(std::move(o.fbb_)),
    compressed_buffer_(std::move(o.compressed_buffer_)),
    meta_(o.meta_),
    finished_(o.finished_),
    headers_(std::move(o.headers_)),
    user_buf_(std::move(o.user_buf_)) {
  header = (o.header);
}

void rpc_envelope::add_dynamic_header(const char *header, const char *value) {
  assert(header != nullptr);
  assert(value != nullptr);
  add_dynamic_header(header, (const uint8_t *)value, strlen(value));
}

void rpc_envelope::add_dynamic_header(const char *header,
                                      const uint8_t *value,
                                      const size_t &value_len) {
  auto k = fbb_->CreateString(header);
  auto v = fbb_->CreateVector(value, value_len);
  auto h = fbs::rpc::CreateDynamicHeader(*fbb_.get(), k, v);
  headers_.push_back(std::move(h));
}


bool rpc_envelope::is_finished() const { return finished_; }
void rpc_envelope::set_request_id(const uint32_t &service,
                                  const uint32_t method) {
  meta_ = service ^ method;
}
void rpc_envelope::set_status(const uint32_t &status) { meta_ = status; }
void rpc_envelope::finish() {
  if(!finished_) {
    // Warning: The headers MUST be VectorOfSortedTables()*
    // This will break binary search if not true
    //
    auto payload = fbs::rpc::CreatePayload(
      *fbb_.get(), meta_, fbb_->CreateVectorOfSortedTables(&headers_),
      user_buf_);
    fbb_->Finish(payload);
    header = header_for_payload((const char *)fbb_->GetBufferPointer(),
                                fbb_->GetSize());
    finished_ = true;
  }
}
void rpc_envelope::init(const uint8_t *buf_to_copy, size_t len) {
  if(buf_to_copy != nullptr && len > 0) {
    user_buf_ = std::move(fbb_->CreateVector(buf_to_copy, len));
  }
}

void rpc_envelope::set_compressed_payload(temporary_buffer<char> buf) {
  compressed_buffer_ = std::move(buf);
  fbb_.reset();
  header =
    header_for_payload(compressed_buffer_.get(), compressed_buffer_.size(),
                       fbs::rpc::Flags::Flags_ZSTD);
}


} // end namespace
