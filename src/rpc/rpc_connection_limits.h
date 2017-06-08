// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
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
struct rpc_connection_limits {
  rpc_connection_limits(uint64_t basic_req_size = 256,
                        uint64_t bloat_mult = 1.57,  // same as folly::vector
                        uint64_t max_mem    = std::max<uint64_t>(
                          0.08 * seastar::memory::stats().total_memory(),
                          uint64_t(1) << 32 /*4GB per core*/));

  /// Minimum request footprint in memory
  const uint64_t basic_request_size;
  /// Serialized size multiplied by this to estimate
  /// memory used by request
  const uint64_t     bloat_factor;
  const uint64_t     max_memory;
  seastar::semaphore resources_available;
  // TODO(agallego) - rename to connection drain accept()
  seastar::gate reply_gate;


  seastar::future<> wait_for_payload_resources(uint64_t payload_size) {
    return wait_for_resources(estimate_request_size(payload_size));
  }
  void release_payload_resources(uint64_t payload_size) {
    release_resources(estimate_request_size(payload_size));
  }

  ~rpc_connection_limits();

  /// \brief - estimates payload + bloat factor
  /// WARNING:
  /// If you use manually, be careful not to leak resources
  /// Please try to use wait_for_payload_resources()
  /// and release_payload_resources()
  uint64_t estimate_request_size(uint64_t serialized_size);
  seastar::future<> wait_for_resources(uint64_t memory_consumed);
  void release_resources(uint64_t memory_consumed);
};
std::ostream &operator<<(std::ostream &o, const rpc_connection_limits &l);

}  // namespace smf
