// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <experimental/optional>
// seastar
#include <core/iostream.hh>
// smf
#include "flatbuffers/rpc_generated.h"
#include "hashing/hashing_utils.h"
#include "platform/log.h"
#include "platform/macros.h"
#include "rpc/rpc_connection.h"
#include "rpc/rpc_connection_limits.h"

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
  static future<rpc_recv_context> parse(rpc_connection *       conn,
                                        rpc_connection_limits *limits);

  /// \brief default ctor
  /// moves in a hdr and body payload after verification. usually passed
  // in via the parse() function above
  rpc_recv_context(temporary_buffer<char> &&hdr, temporary_buffer<char> &&body);
  rpc_recv_context(rpc_recv_context &&o) noexcept;
  ~rpc_recv_context();

  /// \brief used by the server side to determine the actual RPC
  uint32_t request_id();

  /// \brief used by the client side to determine the status from the server
  /// follows the HTTP status codes
  uint32_t status() ;

  /// Notice that this is safe. flatbuffers uses this internally via
  /// `PushBytes()` which is nothing more than
  /// \code
  /// struct foo;
  /// flatbuffers::PushBytes((uint8_t*)foo, sizeof(foo));
  /// \endcode
  /// because the flatbuffers compiler can force only primitive types that
  /// are padded to the largest member size
  /// This is the main reason we are using flatbuffers - no serialization cost
  inline fbs::rpc::Header *header() {
    return reinterpret_cast<fbs::rpc::Header *>(header_buf.get_write());
  };

  /// \bfief casts the byte buffer to a pointer of the payload type
  /// This is what the root_type type; is with flatbuffers. if you
  /// look at the generated code it just calls GetMutableRoot<>
  ///
  /// This is the main reason we are using flatbuffers - no serialization cost
  ///
  inline fbs::rpc::Payload *payload() {
    return flatbuffers::GetMutableRoot<fbs::rpc::Payload>(
      static_cast<void *>(body_buf.get_write()));
  }

  temporary_buffer<char> header_buf;
  temporary_buffer<char> body_buf;
  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_recv_context);
};
}  // namespace smf
