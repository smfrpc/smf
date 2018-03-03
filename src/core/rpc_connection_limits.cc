// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "smf/rpc_connection_limits.h"

#include <fmt/format.h>

#include "smf/human_bytes.h"
#include "smf/log.h"

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

}  // namespace smf
