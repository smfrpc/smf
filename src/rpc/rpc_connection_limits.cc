// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_connection_limits.h"
#include <fmt/format.h>
#include "platform/log.h"
#include "utils/human_bytes_printing_utils.h"

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

seastar::future<> rpc_connection_limits::wait_for_resources(
  size_t memory_consumed) {
  LOG_ERROR_IF(
    memory_consumed > max_memory,
    "memory to serve request `{}`, exceeds max available memory `{}`",
    memory_consumed, max_memory);

  return resources_available.wait(memory_consumed);
}

void rpc_connection_limits::release_resources(size_t memory_consumed) {
  resources_available.signal(memory_consumed);
}
void rpc_connection_limits::release_payload_resources(uint64_t payload_size) {
  release_resources(estimate_request_size(payload_size));
}

seastar::future<> rpc_connection_limits::wait_for_payload_resources(
  uint64_t payload_size) {
  return wait_for_resources(estimate_request_size(payload_size));
}
void rpc_connection_limits::set_body_parsing_timeout() {
  using namespace std::chrono_literals;
  body_timeout_.set_callback([] {
    auto msg = "Parsing the body of the connection took more than 1minute. "
               "Timeout exceeded";
    throw std::runtime_error(msg);
  });
  body_timeout_.arm(1min);
}
void rpc_connection_limits::cancel_body_parsing_timeout() {
  body_timeout_.cancel();
}


rpc_connection_limits::~rpc_connection_limits() {}

std::ostream &operator<<(std::ostream &o, const rpc_connection_limits &l) {
  o << "rpc_connection_limits{'basic_req_size':";
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
