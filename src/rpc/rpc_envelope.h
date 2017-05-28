// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// seastar
#include <core/future.hh>
#include <core/iostream.hh>
// smf
#include "flatbuffers/rpc_generated.h"
#include "platform/macros.h"

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


  /// \brief helper method to turn a flatbuffers::NativeTable
  /// into an envelope
  template <typename T>
  static rpc_envelope native_table_as_envelope(const T &t) {
    static_assert(std::is_base_of<flatbuffers::NativeTable, T>::value,
                  "argument `t' must extend flatbuffers::NativeTable");

#define FBB_FN_BUILD(T) Create##T
    rpc_envelope e(nullptr);
    FBB_FN_BUILD(type)(*e.mutable_builder(), &t);
    return std::move(e);
  }


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


  void set_compressed_payload(temporary_buffer<char> buf);

  temporary_buffer<char> get_compressed_payload() {
    return std::move(compressed_buffer_);
  }

  fbs::rpc::Header header{0, fbs::rpc::Flags::Flags_NONE, 0};

 private:
  /// Sets `letter_.user_buf' to the contents of `src_copy' with size `len'
  /// Also ensurfes that the letter type is `rpc_letter_type_fbb_builder`
  ///
  void init(const uint8_t *src_copy, size_t len);

 private:
  rpc_letter letter_;
  bool       finished = false;
};
}  // namespace smf
