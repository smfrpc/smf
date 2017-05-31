// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <ostream>
// seastar
#include <core/future.hh>
namespace smf {

class rpc_server_stats {
 public:
  uint64_t active_connections{};
  uint64_t total_connections{};
  uint64_t in_bytes{};
  uint64_t out_bytes{};
  uint64_t bad_requests{};
  uint64_t no_route_requests{};
  uint64_t completed_requests{};
  uint64_t too_large_requests{};
  // you need this so you can invoke
  // on a distributed<type> obj_ a map reduce
  // i.e.:
  // obj_.map_reduce(adder<type>, &outer::rpc_server_stats);
  void operator+=(const rpc_server_stats &o);
  rpc_server_stats  self();
  seastar::future<> stop();
};
std::ostream &operator<<(std::ostream &o, const rpc_server_stats &s);
}  // namespace smf
