#pragma once

// std
#include <sstream>
// seastar
#include <core/timer.hh>
// smurf
#include "log.h"
#include "rpc/rpc_server_stats.h"

namespace smf {
class rpc_server_stats_printer {
  public:
  using duration_t = timer<>::duration;
  rpc_server_stats_printer(distributed<rpc_server_stats> &stats,
                    duration_t d = std::chrono::seconds(1))
    : stats_(stats), period_(d) {}

  void start() {
    timer_.set_callback([this] {
      stats().then([this](rpc_server_stats stats) {
        std::stringstream ss;
        ss << "Periodic stats: " << stats << std::endl;
        log.info(ss.str().c_str());
      });
    });
    timer_.arm_periodic(period_);
  }

  future<> stop() { return make_ready_future<>(); }

  private:
  future<rpc_server_stats> stats() {
    // rpc_server_stats needs to support the + operator,
    // the mapreduce framework is per core!
    return stats_.map_reduce(adder<rpc_server_stats>(), &rpc_server_stats::self);
  }
  timer<> timer_;
  distributed<rpc_server_stats> &stats_;
  duration_t period_;
};
}
