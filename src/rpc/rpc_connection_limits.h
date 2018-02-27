// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <chrono>
#include <ostream>
// seastar
#include <core/distributed.hh>
#include <core/gate.hh>

namespace smf {
/// \brief Resource limits for an RPC server
///
/// A request's memory use will be estimated as
///
///     req_mem = basic_request_size + sizeof(serialized_request) * bloat_factor
///
/// Concurrent requests will be limited so that
///
///     sum(req_mem) <= max_memory
///
/// Currently, it contains the limit to prase the body of the connection to be
/// 1minute after successfully parsing the header. If the RPC doesn't finish
/// parsing the  body, it will throw an exception causing a connection close on
/// the server side
///
struct rpc_connection_limits {
  using timer_duration_t = seastar::timer<>::duration;
  rpc_connection_limits(
    uint64_t         basic_req_size        = 256,
    double           bloat_mult            = 1.57,  // same as folly::vector
    uint64_t         max_mem               = uint64_t(1) << 31,  // 2GB per core
    timer_duration_t body_timeout_duration = std::chrono::minutes(1));
  ~rpc_connection_limits() = default;

  /// Minimum request footprint in memory
  const uint64_t basic_request_size;
  /// Serialized size multiplied by this to estimate
  /// memory used by request
  const uint64_t         bloat_factor;
  const uint64_t         max_memory;
  const timer_duration_t max_body_parsing_duration;

  seastar::semaphore resources_available;
  seastar::gate      reply_gate;

  /// \brief releases the resources allocated by `wait_for_payload_resources`
  void release_payload_resources(uint64_t payload_size);
  /// \brief uses a simple formula of (base + serialized_size)
  uint64_t          estimate_request_size(uint64_t serialized_size);

};
std::ostream &operator<<(std::ostream &o, const rpc_connection_limits &l);

}  // namespace smf
