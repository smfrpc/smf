#include "rpc/rpc_connection_limits.h"
#include <fmt/format.h>
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
  if(memory_consumed > max_memory) {
    auto s = fmt::format(
      "memory to serve request `{}`, exceeds max available memory `{}`",
      memory_consumed, max_memory);
    throw std::runtime_error(s.c_str());
  }
  return resources_available.wait(memory_consumed);
}
void rpc_connection_limits::release_resources(size_t memory_consumed) {
  resources_available.signal(memory_consumed);
}

rpc_connection_limits::~rpc_connection_limits() {}

std::ostream &operator<<(std::ostream &o, const rpc_connection_limits &l) {
  o << "{'basic_req_size':" << l.basic_request_size
    << ",'bloat_factor': " << l.bloat_factor << ",'max_mem':" << l.max_memory
    << ",'res_avail':" << l.resources_available.current() << "}";
  return o;
}
} // smf
