#pragma once

// std
#include <sstream>
// seastar
#include <core/timer.hh>
#include <util/log.hh>
// smurf
#include "rpc/rpc_stats.h"

namespace smf {
static seastar::logger log("stats");
class rpc_stats_printer {
  public:
  using duration_t = timer<>::duration;
  rpc_stats_printer(distributed<rpc_stats> &stats,
                    duration_t d = std::chrono::seconds(1))
    : stats_(stats), period_(d) {}

  void start() {
    timer_.set_callback([this] {
      stats().then([this](rpc_stats &&stats) {
        // std::stringstream ss;
        // ss << "Periodic stats: " << stats << std::endl;
        // log.info(ss.str().c_str());
      });
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
}
