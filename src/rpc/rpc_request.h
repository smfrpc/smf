#pragma once
#include <experimental/optional>
// smf
#include "hashing_utils.h"
#include "rpc_generated.h"
namespace smf {
namespace exp = std::experimental;
class rpc_request {
  public:
  rpc_request(uint8_t *buf_to_copy, size_t len) {
    user_buf_ = std::move(fbb_.CreateVector(buf_to_copy, len));
  }
  const uint8_t *buffer() { return fbb_.GetBufferPointer(); }
  size_t length() { return fbb_.GetSize(); }
  const fbs::rpc::Header &header() const { return header_; }

  void add_dynamic_header(const char *header, const char *value) {
    auto k = fbb_.CreateString(header);
    auto v = fbb_.CreateString(value);
    auto h = fbs::rpc::CreateDynamicHeader(fbb_, k, v);
    headers_.push_back(std::move(h));
  }

  void set_oneway(bool oneway) { oneway_ = oneway; }
  bool is_oneway() const { return oneway_; }

  /// \brief returns if the data is finished, useful if you acknowledge that
  /// you might be working w/ unfinished builders/requests
  /// \return - true iff buffer is done building
  bool is_finished() const { return finished_; }


  /// \brief you must call this before you ship it
  /// it will get automatically called for you right before you send the data
  /// over the write by the rpc_client
  void finish() {
    if(!finished_) {
      auto payload = fbs::rpc::CreatePayload(
        fbb_, fbb_.CreateVectorOfSortedTables(&headers_), user_buf_);
      fbb_.Finish(payload);
      post_process_buffer();
    }
  }

  private:
  void post_process_buffer() {
    // assumes that the builder is .Finish()
    using fbs::rpc::Flags;
    auto crc = crc_32((const char *)buffer(), length());
    header_ = fbs::rpc::Header(length(), Flags::Flags_VERIFY_PAYLOAD, crc);
    finished_ = true;
  }

  private:
  // must be first
  flatbuffers::FlatBufferBuilder fbb_;
  bool oneway_ = false;
  bool finished_ = false;
  fbs::rpc::Header header_{0, fbs::rpc::Flags::Flags_NONE, 0};
  std::vector<flatbuffers::Offset<fbs::rpc::DynamicHeader>> headers_{};
  flatbuffers::Offset<flatbuffers::Vector<unsigned char>> user_buf_;
};
}
