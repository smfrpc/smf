// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// seastar
#include <core/distributed.hh>
#include <core/timer.hh>
// smf
#include "rpc/rpc_server_stats.h"

namespace smf {
class rpc_server_stats_printer {
 public:
  using duration_t = seastar::timer<>::duration;
  rpc_server_stats_printer(seastar::distributed<rpc_server_stats> *stats,
                           duration_t d = std::chrono::minutes(2));

  void              start();
  seastar::future<> stop();

 private:
  /// \brief calls map_reduce on distributed<stats>
  ///
  seastar::future<rpc_server_stats> aggregate_stats();

 private:
  seastar::timer<>                        timer_;
  seastar::distributed<rpc_server_stats> *stats_;
  duration_t                              period_;
};
}  // namespace smf
