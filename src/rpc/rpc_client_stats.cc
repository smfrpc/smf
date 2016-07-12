#include "rpc/rpc_client_stats.h"

namespace smf {
void rpc_client_stats::operator+=(const rpc_client_stats &o) {
  in_bytes += o.in_bytes;
  out_bytes += o.out_bytes;
  bad_requests += o.bad_requests;
  completed_requests += o.completed_requests;
}
rpc_client_stats rpc_client_stats::self() {
  return *this; // make a copy for map_reduce framework
}
future<> rpc_client_stats::stop() { return make_ready_future<>(); }
std::ostream &operator<<(std::ostream &o, const rpc_client_stats &s) {
  o << "{'in_bytes': " << s.in_bytes << ","
    << "'out_bytes': " << s.out_bytes << ","
    << "'bad_reqs': " << s.bad_requests << ","
    << "'completed_reqs': " << s.completed_requests << "}";
  return o;
}
}
