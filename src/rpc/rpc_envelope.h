#pragma once
// seastar
#include <core/future.hh>
#include <core/iostream.hh>
// smf
#include "rpc_generated.h"
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
  static future<> send(output_stream<char> &out, temporary_buffer<char> req);

  /// \brief convenience method. copy the byte array of the flatbuffer builder
  /// \args buf_to_copy - is a pointer to the flatbuffers after the user called
  /// fbb->finished()
  rpc_envelope(const flatbuffers::FlatBufferBuilder &fbb);

  /// \brief copy the byte array to internal data structures.
  /// \args buf_to_copy - is a pointer to the byte array to send to remote
  /// convenience method for sending literals. i.e.: rpc_envelope("hello")
  ///
  rpc_envelope(const char *buf_to_copy);

  /// \brief copy the byte array to internal data structures.
  /// \args buf_to_copy - is a pointer to the byte array to send to remote
  /// convenience method for not having to manually cast between seastar &
  /// flatbuffers
  ///
  rpc_envelope(const char *buf_to_copy, size_t len);

  /// \brief copy the byte array to internal data structures.
  /// \args buf_to_copy - is a pointer to the byte array to send to remote
  /// \args len - length of the buf_to_copy. Note that ther is no validation
  ///
  rpc_envelope(const uint8_t *buf_to_copy, size_t len);


  /// \brief copy ctor deleted
  rpc_envelope(const rpc_envelope &o) = delete;

  /// \brief move ctor
  rpc_envelope(rpc_envelope &&o) noexcept;

  /// \brief approx size unreliable unless finished() is called
  size_t size();

  /// \brief convenience method for transofrming thi into tmpbuf
  ///
  temporary_buffer<char> to_temp_buf();

  /// \brief add a key=value pair ala HTTP/1.1 so that the decoding
  /// end has meta information about the request. For example, specify the
  /// \code `Content-Type: json` \endcode , etc.
  /// Useful for the framework to send trace information etc.
  ///
  void add_dynamic_header(const char *header, const char *value);

  /// brief add a key=value pair. the value is always binary up to the user how
  /// to represent
  void add_dynamic_header(const char *header,
                          const uint8_t *value,
                          const size_t &value_len);

  /// \brief if the user doesn't want to get a callback, or receive data
  /// back from the server after the requeset. That is use TCP as the
  /// best effort delivery. No need for application acknowledgement
  ///
  void set_oneway(bool oneway);

  /// \brief used to test if no need for ack
  /// \return true iff no need for ack
  ///
  bool is_oneway() const;

  /// \brief returns if the data is finished, useful if you acknowledge that
  /// you might be working w/ unfinished builders/requests
  /// \return - true iff buffer is done building
  bool is_finished() const;
  void set_request_id(const uint32_t &service, const uint32_t method);
  void set_status(const uint32_t &status);

  /// \brief you must call this before you ship it
  /// it will get automatically called for you right before you send the data
  /// over the write by the rpc_client
  void finish();

  private:
  ///\brief shared initialization
  void init(const uint8_t *buf_to_copy, size_t len);

  /// \brief sets the finished_=true as the main contribution
  /// after guaranteeing that the payload has been checksum'med
  /// length sepcify and other connection specific headers
  ///
  void post_process_buffer();


  private:
  // must be first
  std::unique_ptr<flatbuffers::FlatBufferBuilder> fbb_ =
    std::make_unique<flatbuffers::FlatBufferBuilder>();
  uint32_t meta_ = 0;
  bool oneway_ = false;
  bool finished_ = false;
  fbs::rpc::Header header_{0, fbs::rpc::Flags::Flags_NONE, 0};
  std::vector<flatbuffers::Offset<fbs::rpc::DynamicHeader>> headers_{};
  flatbuffers::Offset<flatbuffers::Vector<unsigned char>> user_buf_;
};
}
