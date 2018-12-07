// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <chrono>
#include <ostream>

#include <seastar/core/semaphore.hh>
#include <seastar/core/timer.hh>

#include <smf/human_bytes.h>

namespace smf {
/// Currently, it contains the limit to prase the body of the connection to be
/// 1minute after successfully parsing the header. If the RPC doesn't finish
/// parsing the  body, it will throw an exception causing a connection close on
/// the server side
///
struct rpc_connection_limits {
  using timer_duration_t = seastar::timer<>::duration;
  explicit rpc_connection_limits(uint64_t max_mem_per_core,
                                 timer_duration_t body_timeout_duration)
    : max_memory(max_mem_per_core),
      max_body_parsing_duration(body_timeout_duration),
      resources_available(max_mem_per_core) {}

  ~rpc_connection_limits() = default;

  const uint64_t max_memory;
  const timer_duration_t max_body_parsing_duration;

  seastar::semaphore resources_available;
};
inline std::ostream &
operator<<(std::ostream &o, const ::smf::rpc_connection_limits &l) {
  o << "rpc_connection_limits{max_mem:" << ::smf::human_bytes(l.max_memory)
    << ", max_body_parsing_duration: "
    << std::chrono::duration_cast<std::chrono::milliseconds>(
         l.max_body_parsing_duration)
         .count()
    << "ms, res_avail:" << ::smf::human_bytes(l.resources_available.current())
    << " (" << l.resources_available.current() << ")}";
  return o;
}

}  // namespace smf
