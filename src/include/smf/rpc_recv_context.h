// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
// seastar
#include <seastar/core/iostream.hh>
#include <seastar/net/api.hh>
// smf
#include "smf/macros.h"
#include "smf/rpc_connection.h"
#include "smf/rpc_connection_limits.h"
#include "smf/rpc_generated.h"
#include "smf/std-compat.h"

namespace smf {
struct rpc_recv_context {
  /// \brief determines if we've correctly parsed the request
  /// \return  optional fully parsed request, iff the request is supported
  /// i.e: we support the compression algorithm, etc
  ///
  /// Parses the incoming rpc in 2 steps. First we statically know the
  /// size of the header, so we parse sizeof(Header). We with this information
  /// we parse the body of the request
  ///
  static seastar::future<smf::compat::optional<rpc::header>>
  parse_header(rpc_connection *conn);
  static seastar::future<smf::compat::optional<rpc_recv_context>>
  parse_payload(rpc_connection *conn, rpc::header hdr);

  explicit rpc_recv_context(
    seastar::lw_shared_ptr<rpc_connection_limits> server_instance_limits,
    seastar::socket_address remote_address, rpc::header hdr,
    seastar::temporary_buffer<char> body);
  rpc_recv_context(rpc_recv_context &&o) noexcept;
  ~rpc_recv_context();

  /// \brief used by the server side to determine the actual RPC
  SMF_ALWAYS_INLINE uint32_t
  request_id() const {
    return header.meta();
  }

  /// \brief used by the client side to determine the status from the server
  /// follows the HTTP status codes
  SMF_ALWAYS_INLINE uint32_t
  status() const {
    return header.meta();
  }

  SMF_ALWAYS_INLINE uint16_t
  session() const {
    return header.session();
  }

  seastar::lw_shared_ptr<rpc_connection_limits> rpc_server_limits;
  const seastar::socket_address remote_address;
  rpc::header header;
  seastar::temporary_buffer<char> payload;
  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_recv_context);
};
}  // namespace smf
