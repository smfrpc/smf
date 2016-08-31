#include "rpc/rpc_envelope.h"
#include <cstring>
// compression
#include <zstd.h>
// smf
#include "hashing_utils.h"
#include "log.h"

namespace smf {

fbs::rpc::Header
header_for_payload(const char *payload,
                   size_t payload_size,
                   fbs::rpc::Flags flags = fbs::rpc::Flags::Flags_NONE) {
  using fbs::rpc::Flags;
  auto crc = xxhash(payload, payload_size);
  uint32_t header_flags =
    static_cast<uint32_t>(Flags::Flags_CHECKSUM) | static_cast<uint32_t>(flags);
  return fbs::rpc::Header(payload_size, static_cast<Flags>(header_flags), crc);
}

constexpr static size_t kHeaderSize = sizeof(fbs::rpc::Header);

future<> rpc_envelope::send(output_stream<char> &out,
                            temporary_buffer<char> buf) {
  return out.write(std::move(buf)).then([&out] { return out.flush(); });
}

void rpc_envelope::set_dynamic_compression_min_size(uint32_t i) {
  if(i >= 1000) {
    dynamic_compression_min_size_ = i;
  }
}
bool rpc_envelope::get_dynamic_compression() const {
  return dynamic_compression_;
}
void rpc_envelope::set_dynamic_compression(bool b) { dynamic_compression_ = b; }

temporary_buffer<char> rpc_envelope::to_temp_buf() {
  this->finish();
  temporary_buffer<char> buf(size());

  if(dynamic_compression_ && fbb_->GetSize() >= dynamic_compression_min_size_) {
    // zstd iface is about void* - that's life
    // leave room for header
    void *dst = static_cast<void *>(buf.get_write() + kHeaderSize);
    const void *src = static_cast<const void *>(fbb_->GetBufferPointer());
    auto zstd_compressed_size =
      ZSTD_compress(dst, buf.size() - kHeaderSize, src, fbb_->GetSize(),
                    3 /*default compression level*/);

    auto zstd_err = ZSTD_isError(zstd_compressed_size);
    if(zstd_err == 0) {
      buf.trim(zstd_compressed_size + kHeaderSize);
      // The payload starts after the header. Save enough bytes for the header
      // and compute the checksum on the body only.
      auto header_cpy =
        header_for_payload(buf.get() + kHeaderSize, zstd_compressed_size,
                           fbs::rpc::Flags::Flags_ZSTD);
      // copy the remaining header bytes
      std::copy((char *)&header_cpy, (char *)&header_cpy + kHeaderSize,
                buf.get_write());
      return std::move(buf);
    } else {
      LOG_ERROR("Error compressing zstd buffer. defaulting to uncompressed. "
                "Code: {}, Desciption: {}",
                zstd_err, ZSTD_getErrorName(zstd_err));
    }
  }
  // default
  std::copy((char *)&header_, (char *)&header_ + kHeaderSize, buf.get_write());
  std::copy((char *)fbb_->GetBufferPointer(),
            (char *)fbb_->GetBufferPointer() + fbb_->GetSize(),
            buf.get_write() + kHeaderSize);
  return std::move(buf);
}


size_t rpc_envelope::size() { return kHeaderSize + fbb_->GetSize(); }

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
    meta_(o.meta_),
    oneway_(o.oneway_),
    finished_(o.finished_),
    header_(std::move(o.header_)),
    headers_(std::move(o.headers_)),
    user_buf_(std::move(o.user_buf_)) {}

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


void rpc_envelope::set_oneway(bool oneway) { oneway_ = oneway; }
bool rpc_envelope::is_oneway() const { return oneway_; }
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
    post_process_buffer();
  }
}
void rpc_envelope::init(const uint8_t *buf_to_copy, size_t len) {
  if(buf_to_copy != nullptr && len > 0) {
    user_buf_ = std::move(fbb_->CreateVector(buf_to_copy, len));
  }
}

void rpc_envelope::post_process_buffer() {
  // assumes that the builder is .Finish()
  header_ =
    header_for_payload((const char *)fbb_->GetBufferPointer(), fbb_->GetSize());
  finished_ = true;
}
} // end namespace
