// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_connection_limits.h"
#include <fmt/format.h>
#include "utils/human_bytes_printing_utils.h"
#include "platform/log.h"

namespace smf {

rpc_connection_limits::rpc_connection_limits(size_t basic_req_size,
                                             size_t bloat_mult,
                                             size_t max_mem)
  : basic_request_size(basic_req_size)
  , bloat_factor(bloat_mult)
  , max_memory(max_mem)
  , resources_available(max_mem) {}

size_t rpc_connection_limits::estimate_request_size(size_t serialized_size) {
  return (basic_request_size + serialized_size) * bloat_factor;
}

future<> rpc_connection_limits::wait_for_resources(size_t memory_consumed) {
  LOG_THROW_IF(
    memory_consumed > max_memory,
    "memory to serve request `{}`, exceeds max available memory `{}`",
    memory_consumed, max_memory);

  return resources_available.wait(memory_consumed);
}

void rpc_connection_limits::release_resources(size_t memory_consumed) {
  resources_available.signal(memory_consumed);
}

rpc_connection_limits::~rpc_connection_limits() {}

std::ostream &operator<<(std::ostream &o, const rpc_connection_limits &l) {
  o << "{'basic_req_size':";
  human_bytes<uint64_t>(o, l.basic_request_size);
  o << ",'bloat_factor': ";
  human_bytes<uint64_t>(o, l.bloat_factor);
  o << ",'max_mem':";
  human_bytes<uint64_t>(o, l.max_memory);
  o << ",'res_avail':";
  human_bytes<uint64_t>(o, l.resources_available.current());
  o << "}";
  return o;
}
}  // namespace smf
