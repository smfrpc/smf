#pragma once
#include <experimental/optional>
// smf
#include "hashing_utils.h"
#include "rpc_generated.h"
namespace smf {
namespace exp = std::experimental;

/// \brief send rpc request to stablished remote host.
/// This class will async send the request to the remote host in rpc_client.h
/// and will ensure that the request is valid, enabling tracing if available,
/// compression, etc. Send arbitrary byte arrays to remote host
///
class rpc_request {
  public:
  /// \brief copy the byte array to internal data structures.
  /// \args buf_to_copy - is a pointer to the byte array to send to remote
  /// \args len - length of the buf_to_copy. Note that ther is no validation
  ///
  rpc_request(const uint8_t *buf_to_copy, size_t len) {
    // TODO(agallego) - check if the len is greater than compression limit
    // and if it is, then snappy compress the byte buffer and set the
    // this->dynamic_header("Content-Encoding", "snappy");
    //
    user_buf_ = std::move(
      fbb_->CreateVector(buf_to_copy, buf_to_copy == nullptr ? 0 : len));
  }
  /// \brief pointer to the underlying byte buffer. This SHOULD
  /// be call after finished() is called. Check if safe to call via
  /// `is_finished()`
  /// \return a pointer to the byte buffer
  ///
  const uint8_t *buffer() { return fbb_->GetBufferPointer(); }

  /// \brief length to accompany the buffer() method call.
  /// This MUST be called AFTER `finished()` it is unsafe to use before
  /// \return the length of the byte buffer
  ///
  size_t length() { return fbb_->GetSize(); }

  /// \brief this will be nil (set to 0's) if not finished()
  /// \return the header describing the byte buffer backing the request
  ///
  const fbs::rpc::Header &header() const { return header_; }


  /// \brief add a key=value pair ala HTTP/1.1 so that the decoding
  /// end has meta information about the request. For example, specify the
  /// \code `Content-Type: json` \endcode , etc.
  /// Useful for the framework to send trace information etc.
  ///
  void add_dynamic_header(const char *header, const char *value) {
    auto k = fbb_->CreateString(header);
    auto v = fbb_->CreateString(value);
    auto h = fbs::rpc::CreateDynamicHeader(*fbb_.get(), k, v);
    headers_.push_back(std::move(h));
  }

  /// \brief if the user doesn't want to get a callback, or receive data
  /// back from the server after the requeset. That is use TCP as the
  /// best effort delivery. No need for application acknowledgement
  ///
  void set_oneway(bool oneway) { oneway_ = oneway; }

  /// \brief used to test if no need for ack
  /// \return true iff no need for ack
  ///
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
        *fbb_.get(), fbb_->CreateVectorOfSortedTables(&headers_), user_buf_);
      fbb_->Finish(payload);
      post_process_buffer();
    }
  }

  private:
  /// \brief sets the finished_=true as the main contribution
  /// after guaranteeing that the payload has been crc32'ed
  /// length sepcify and other connection specific headers
  ///
  void post_process_buffer() {
    // assumes that the builder is .Finish()
    using fbs::rpc::Flags;
    auto crc = crc_32((const char *)buffer(), length());
    header_ = fbs::rpc::Header(length(), Flags::Flags_VERIFY_PAYLOAD, crc);
    finished_ = true;
  }

  private:
  // must be first
  std::unique_ptr<flatbuffers::FlatBufferBuilder> fbb_ =
    std::make_unique<flatbuffers::FlatBufferBuilder>();
  bool oneway_ = false;
  bool finished_ = false;
  fbs::rpc::Header header_{0, fbs::rpc::Flags::Flags_NONE, 0};
  std::vector<flatbuffers::Offset<fbs::rpc::DynamicHeader>> headers_{};
  flatbuffers::Offset<flatbuffers::Vector<unsigned char>> user_buf_;
};
}
