#include "rpc/rpc_server_stats.h"
namespace smf {
void rpc_server_stats::operator+=(const rpc_server_stats &o) {
  active_connections += o.active_connections;
  total_connections += o.total_connections;
  in_bytes += o.in_bytes;
  out_bytes += o.out_bytes;
  bad_requests += o.bad_requests;
  completed_requests += o.completed_requests;
  too_large_requests += o.too_large_requests;
}
rpc_server_stats rpc_server_stats::self() {
  return *this; // make a copy for map_reduce framework
}

future<> rpc_server_stats::stop() { return make_ready_future<>(); }


std::ostream &operator<<(std::ostream &o, const rpc_server_stats &s) {
  o << "active conn: " << s.active_connections << ", "
    << "total conn: " << s.total_connections << ", "
    << "in bytes: " << s.in_bytes << ", "
    << "out bytes: " << s.out_bytes << ", "
    << "bad requests: " << s.bad_requests << ", "
    << "completed requests: " << s.completed_requests << ", "
    << "too large requests: " << s.too_large_requests;
  return o;
}
}
