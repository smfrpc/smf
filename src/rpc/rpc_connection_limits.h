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
  rpc_connection_limits(
    size_t basic_req_size = 1024,
    size_t bloat_mult = 1.57, // same as folly::vector
    /// The defaults are the ones from scylladb as of 8e124b3a
    size_t max_mem = std::max<size_t>(0.08 * memory::stats().total_memory(),
                                      1000000));

  /// Minimum request footprint in memory
  const size_t basic_request_size;
  /// Serialized size multiplied by this to estimate
  /// memory used by request
  const size_t bloat_factor;
  const size_t max_memory;
  semaphore resources_available;
  // TODO(agallego) - rename to connection drain accept()
  seastar::gate reply_gate;


  size_t estimate_request_size(size_t serialized_size);
  future<> wait_for_resources(size_t memory_consumed);
  void release_resources(size_t memory_consumed);
  ~rpc_connection_limits();
};
std::ostream &operator<<(std::ostream &o, const rpc_connection_limits &l);
} // smf
