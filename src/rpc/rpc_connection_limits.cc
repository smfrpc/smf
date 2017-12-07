// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_connection_limits.h"
#include <fmt/format.h>
#include "platform/log.h"
#include "utils/human_bytes.h"

namespace smf {

rpc_connection_limits::rpc_connection_limits(
  uint64_t                                _basic_req_size,
  double                                  _bloat_mult,
  uint64_t                                _max_mem,
  rpc_connection_limits::timer_duration_t _body_duration)
  : basic_request_size(_basic_req_size)
  , bloat_factor(_bloat_mult)
  , max_memory(_max_mem)
  , max_body_parsing_duration(_body_duration)
  , resources_available(_max_mem) {}

uint64_t
rpc_connection_limits::estimate_request_size(uint64_t serialized_size) {
  return (basic_request_size + serialized_size) * bloat_factor;
}

seastar::future<>
rpc_connection_limits::wait_for_resources(uint64_t memory_consumed) {
  LOG_ERROR_IF(
    memory_consumed > max_memory,
    "memory to serve request `{}`, exceeds max available memory `{}`",
    memory_consumed, max_memory);

  return resources_available.wait(memory_consumed);
}

void
rpc_connection_limits::release_resources(uint64_t memory_consumed) {
  resources_available.signal(memory_consumed);
}
void
rpc_connection_limits::release_payload_resources(uint64_t payload_size) {
  release_resources(estimate_request_size(payload_size));
}

seastar::future<>
rpc_connection_limits::wait_for_payload_resources(uint64_t payload_size) {
  return wait_for_resources(estimate_request_size(payload_size));
}

rpc_connection_limits::~rpc_connection_limits() {}

std::ostream &
operator<<(std::ostream &o, const rpc_connection_limits &l) {
  o << "rpc_connection_limits{'basic_req_size':"
    << human_bytes(l.basic_request_size)
    << ",'bloat_factor': " << human_bytes(l.bloat_factor)
    << ",'max_mem':" << human_bytes(l.max_memory)
    << ",'res_avail':" << human_bytes(l.resources_available.current()) << "( "
    << l.resources_available.current() << " )}";
  return o;
}
}  // namespace smf
