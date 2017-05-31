// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <ostream>
// seastar
#include <core/future.hh>

namespace smf {
class rpc_client_stats {
 public:
  uint64_t in_bytes{};
  uint64_t out_bytes{};
  uint64_t bad_requests{};
  uint64_t completed_requests{};
  // you need this so you can invoke
  // on a distributed<type> obj_ a map reduce
  // i.e.:
  // obj_.map_reduce(adder<type>, &outer::rpc_client_stats);
  void operator+=(const rpc_client_stats &o);
  rpc_client_stats  self();
  seastar::future<> stop();
};

std::ostream &operator<<(std::ostream &o, const rpc_client_stats &s);

}  // namespace smf
