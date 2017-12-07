// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <experimental/optional>
// seastar
#include <core/iostream.hh>
// smf
#include "flatbuffers/fbs_typed_buf.h"
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
  /// i.e: we support the compression algorithm, etc
  ///
  /// Parses the incoming rpc in 2 steps. First we statically know the
  /// size of the header, so we parse sizeof(Header). We with this information
  /// we parse the body of the request
  ///
  static seastar::future<exp::optional<rpc_recv_context>> parse(
    rpc_connection *conn);

  rpc_recv_context(rpc::header hdr, seastar::temporary_buffer<char> body);
  rpc_recv_context(rpc_recv_context &&o) noexcept;
  ~rpc_recv_context();

  /// \brief used by the server side to determine the actual RPC
  uint32_t request_id() const;

  /// \brief used by the client side to determine the status from the server
  /// follows the HTTP status codes
  uint32_t status() const;

  uint16_t
  session() const {
    return header.session();
  }

  rpc::header                     header;
  seastar::temporary_buffer<char> payload;
  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_recv_context);
};
}  // namespace smf
