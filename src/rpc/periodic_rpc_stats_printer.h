#pragma once
// include timer from seastar
#include "rpc/rpc_stats.h"

class stats_printer {
  public:
  using duration_t = timer<>::duration;
  stats_printer(distributed<rpc_stats> &stats,
                duration_t d = std::chrono::seconds(1))
    : stats_(stats), period_(d) {}

  void start() {
    timer_.set_callback([this] {
      stats().then([this](auto stats) { std::cout << stats << std::endl; });
    });
    timer_.arm_periodic(period_);
  }

  future<> stop() { return make_ready_future<>(); }

  private:
  future<rpc_stats> stats() {
    // rpc_stats needs to support the + operator,
    // the mapreduce framework is per core!
    return stats_.map_reduce(adder<rpc_stats>(), &rpc_stats::self);
  }
  timer<> timer_;
  distributed<rpc_stats> &stats_;
  duration_t period_;
};
