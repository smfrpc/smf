#include "rpc/rpc_envelope.h"
#include <cstring>
// smf
#include "hashing_utils.h"

// FIXME - delete log
#include "log.h"

namespace smf {
future<> rpc_envelope::send(output_stream<char> &out,
                            temporary_buffer<char> buf) {
  return out.write(std::move(buf)).then([&out] { return out.flush(); });
}

temporary_buffer<char> rpc_envelope::to_temp_buf() {
  size_t hdr_size = sizeof(fbs::rpc::Header);
  this->finish();
  temporary_buffer<char> buf(hdr_size + fbb_->GetSize());
  std::copy((char *)&header_, (char *)&header_ + hdr_size, buf.get_write());
  std::copy((char *)fbb_->GetBufferPointer(),
            (char *)fbb_->GetBufferPointer() + fbb_->GetSize(),
            buf.get_write() + hdr_size);
  return std::move(buf);
}

size_t rpc_envelope::size() {
  static size_t hdr_size = sizeof(fbs::rpc::Header);
  return hdr_size + fbb_->GetSize();
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
  using fbs::rpc::Flags;
  auto crc = crc_32((const char *)fbb_->GetBufferPointer(), fbb_->GetSize());
  header_ = fbs::rpc::Header(fbb_->GetSize(), Flags::Flags_VERIFY_PAYLOAD, crc);
  finished_ = true;
}
} // end namespace
