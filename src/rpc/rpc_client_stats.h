#pragma once
// std
#include <ostream>
// seastar
#include <core/future.hh>

namespace smf {
class rpc_client_stats {
  public:
  size_t in_bytes{};
  size_t out_bytes{};
  size_t bad_requests{};
  size_t completed_requests{};
  // you need this so you can invoke
  // on a distributed<type> obj_ a map reduce
  // i.e.:
  // obj_.map_reduce(adder<type>, &outer::rpc_client_stats);
  void operator+=(const rpc_client_stats &o);
  rpc_client_stats self();
  future<> stop();
};
std::ostream &operator<<(std::ostream &o, const rpc_client_stats &s);
}
