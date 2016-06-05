#pragma once
#include <ostream>

namespace smf {

class rpc_client_stats {
  public:
  size_t in_bytes{};
  size_t out_bytes{};
  size_t bad_requests{};
  size_t completed_requests{};

  time_point last_request;
  // you need this so you can invoke
  // on a distributed<type> obj_ a map reduce
  // i.e.:
  // obj_.map_reduce(adder<type>, &outer::rpc_client_stats);
  void operator+=(const rpc_client_stats &o) {
    active_connections += o.active_connections;
    total_connections += o.total_connections;
    in_bytes += o.in_bytes;
    out_bytes += o.out_bytes;
    bad_requests += o.bad_requests;
    completed_requests += o.completed_requests;
    too_large_requests += o.too_large_requests;
  }
  rpc_client_stats self() {
    return *this; // make a copy for map_reduce framework
  }

  future<> stop() { return make_ready_future<>(); }
};

std::ostream &operator<<(std::ostream &o, const rpc_client_stats &s) {
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
