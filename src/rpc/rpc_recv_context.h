#pragma once
// std
#include <experimental/optional>
// seastar
#include <core/iostream.hh>
// smf
#include "log.h"
#include "hashing_utils.h"
#include "flatbuffers/rpc_generated.h"


namespace smf {
namespace exp = std::experimental;
struct rpc_recv_context {
  /// \brief determines if we've correctly parsed the request
  /// \return  optional fully parsed request, iff the request is supported
  /// i.e: we support the compression algorithm, crc32 etc
  ///
  /// Parses the incoming rpc in 2 steps. First we statically know the
  /// size of the header, so we parse sizeof(Header). We with this information
  /// we parse the body of the request
  ///
  static future<exp::optional<rpc_recv_context>> parse(input_stream<char> &in);

  /// \brief default ctor
  /// moves in a hdr and body payload after verification. usually passed
  // in via the parse() function above
  rpc_recv_context(temporary_buffer<char> hdr, temporary_buffer<char> body);
  rpc_recv_context(rpc_recv_context &&o) noexcept;
  rpc_recv_context(const rpc_recv_context &o) = delete;
  ~rpc_recv_context();
  /// \brief used by the server side to determine the actual RPC
  uint32_t request_id() const;

  /// \brief used by the client side to determine the status from the server
  /// follows the HTTP status codes
  uint32_t status() const;

  temporary_buffer<char> header_buf;
  temporary_buffer<char> body_buf;
  /// Notice that this is safe. flatbuffers uses this internally via
  /// `PushBytes()` which is nothing more than
  /// \code
  /// struct foo;
  /// flatbuffers::PushBytes((uint8_t*)foo, sizeof(foo));
  /// \endcode
  /// because the flatbuffers compiler can force only primitive types that
  /// are padded to the largest member size
  /// This is the main reason we are using flatbuffers - no serialization cost
  fbs::rpc::Header *header;
  /// \bfief casts the byte buffer to a pointer of the payload type
  /// This is what the root_type type; is with flatbuffers. if you
  /// look at the generated code it just calls GetMutableRoot<>
  ///
  /// This is the main reason we are using flatbuffers - no serialization cost
  ///
  fbs::rpc::Payload *payload;
};
}
