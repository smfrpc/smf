#pragma once
// std
#include <ostream>
// seastar
#include <core/future.hh>
namespace smf {

class rpc_server_stats {
  public:
  size_t active_connections{};
  size_t total_connections{};
  size_t in_bytes{};
  size_t out_bytes{};
  size_t bad_requests{};
  size_t completed_requests{};
  size_t too_large_requests{};
  // you need this so you can invoke
  // on a distributed<type> obj_ a map reduce
  // i.e.:
  // obj_.map_reduce(adder<type>, &outer::rpc_server_stats);
  void operator+=(const rpc_server_stats &o);
  rpc_server_stats self();
  future<> stop();
};
std::ostream &operator<<(std::ostream &o, const rpc_server_stats &s);
}
