#pragma once
// seastar
#include <core/timer.hh>
#include <core/distributed.hh>
// smf
#include "rpc/rpc_server_stats.h"

namespace smf {
class rpc_server_stats_printer {
  public:
  using duration_t = timer<>::duration;
  rpc_server_stats_printer(distributed<rpc_server_stats> &stats,
                           duration_t d = std::chrono::seconds(1));

  void start();
  future<> stop();

  private:
  future<rpc_server_stats> stats();

  private:
  timer<> timer_;
  distributed<rpc_server_stats> &stats_;
  duration_t period_;
};
}
