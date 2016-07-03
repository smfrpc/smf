#include "rpc/rpc_envelope.h"
#include <cstring>
// smf
#include "hashing_utils.h"
namespace smf {
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
const uint8_t *rpc_envelope::buffer() { return fbb_->GetBufferPointer(); }
size_t rpc_envelope::length() { return fbb_->GetSize(); }
const fbs::rpc::Header &rpc_envelope::header() const { return header_; }
void rpc_envelope::add_dynamic_header(const char *header, const char *value) {
  auto k = fbb_->CreateString(header);
  auto v = fbb_->CreateString(value);
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
  auto crc = crc_32((const char *)buffer(), length());
  header_ = fbs::rpc::Header(length(), Flags::Flags_VERIFY_PAYLOAD, crc);
  finished_ = true;
}
} // end namespace
