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
  using duration_t = timer<>::duration;
  rpc_server_stats_printer(distributed<rpc_server_stats> *stats,
                           duration_t d = std::chrono::minutes(2));

  void     start();
  future<> stop();

 private:
  /// \brief calls map_reduce on distributed<stats>
  ///
  future<rpc_server_stats> aggregate_stats();

 private:
  timer<>                        timer_;
  uint32_t                       timer_callback_counter_{0};
  distributed<rpc_server_stats> *stats_;
  duration_t                     period_;
};
}  // namespace smf
