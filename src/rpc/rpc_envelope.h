// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// seastar
#include <core/future.hh>
#include <core/iostream.hh>
// smf
#include "flatbuffers/rpc_generated.h"
#include "platform/macros.h"

// Note: This class has no dependency on seastar so that we can
// re-use this class w/ the folly::wangle rpc from non-seastar applications
// don't bring in the seastar dependency
namespace smf {
/// \brief send rpc request to stablished remote host.
/// This class will async send the request to the remote host in rpc_client.h
/// and will ensure that the request is valid, enabling tracing if available,
/// compression, etc. Send arbitrary byte arrays to remote host
///
class rpc_envelope {
 public:
  constexpr static size_t kHeaderSize = sizeof(fbs::rpc::Header);

  static future<> send(output_stream<char> *out, rpc_envelope req);


  /// \brief convenience method. copy the byte array of the flatbuffer builder
  /// \args buf_to_copy - is a pointer to the flatbuffers after the user called
  /// fbb->finished()
  explicit rpc_envelope(const flatbuffers::FlatBufferBuilder &fbb);

  /// \brief copy the byte array to internal data structures.
  /// \args buf_to_copy - is a pointer to the byte array to send to remote
  /// convenience method for sending literals. i.e.: rpc_envelope("hello")
  ///
  explicit rpc_envelope(const char *buf_to_copy);

  /// \brief copy the byte array to internal data structures.
  /// \args buf_to_copy - is a pointer to the byte array to send to remote
  /// convenience method for not having to manually cast between seastar &
  /// flatbuffers
  ///
  rpc_envelope(const char *buf_to_copy, size_t len);

  /// \brief copy the sstring to internal data structures.
  /// \args buf_to_copy - is a ref to the sstring to send to remote
  /// convenience method for not having to manually cast between seastar &
  /// flatbuffers
  ///
  explicit rpc_envelope(const sstring &buf_to_copy);

  /// \brief copy the byte array to internal data structures.
  /// \args buf_to_copy - is a pointer to the byte array to send to remote
  /// \args len - length of the buf_to_copy. Note that ther is no validation
  ///
  rpc_envelope(const uint8_t *buf_to_copy, size_t len);


  /// \brief copy ctor deleted
  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_envelope);

  /// \brief move ctor
  rpc_envelope(rpc_envelope &&o) noexcept;
  rpc_envelope &operator=(rpc_envelope &&o) noexcept;

  /// \brief returns the size of the payload excluding headers
  size_t payload_size() { return fbb_->GetSize(); }

  /// \brief returns an immutable pointer to the buffer
  const uint8_t *payload() { return fbb_->GetBufferPointer(); }


  /// \brief add a key=value pair ala HTTP/1.1 so that the decoding
  /// end has meta information about the request. For example, specify the
  /// \code `Content-Type: json` \endcode , etc.
  /// Useful for the framework to send trace information etc.
  ///
  void add_dynamic_header(const char *header, const char *value);

  /// brief add a key=value pair. the value is always binary up to the user how
  /// to represent
  void add_dynamic_header(const char *   header,
                          const uint8_t *value,
                          const size_t & value_len);

  /// \brief returns if the data is finished, useful if you acknowledge that
  /// you might be working w/ unfinished builders/requests
  /// \return - true iff buffer is done building
  bool is_finished() const;
  void set_request_id(const uint32_t &service, const uint32_t method);
  void set_status(const uint32_t &status);

  inline flatbuffers::FlatBufferBuilder *mutable_builder() {
    return fbb_.get();
  }

  /// \brief you must call this before you ship it
  /// it will get automatically called for you right before you send the data
  /// over the write by the rpc_client
  void finish();

  fbs::rpc::Header header{0, fbs::rpc::Flags::Flags_NONE, 0};


  void set_compressed_payload(temporary_buffer<char> buf);

  temporary_buffer<char> get_compressed_payload() {
    return std::move(compressed_buffer_);
  }

 private:
  /// \brief shared initialization
  void init(const uint8_t *buf_to_copy, size_t len);

 private:
  std::unique_ptr<flatbuffers::FlatBufferBuilder> fbb_ =
    std::make_unique<flatbuffers::FlatBufferBuilder>();

  temporary_buffer<char> compressed_buffer_;
  uint32_t               meta_     = 0;
  bool                   finished_ = false;

  std::vector<flatbuffers::Offset<fbs::rpc::DynamicHeader>> headers_{};
  flatbuffers::Offset<flatbuffers::Vector<unsigned char>>   user_buf_;
};
}  // namespace smf
