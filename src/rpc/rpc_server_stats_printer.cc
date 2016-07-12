#include "rpc/rpc_server_stats_printer.h"
#include <sstream>
#include "log.h"

namespace smf {
using duration_t = timer<>::duration;
rpc_server_stats_printer::rpc_server_stats_printer(
  distributed<rpc_server_stats> &stats, duration_t d)
  : stats_(stats), period_(d) {}

void rpc_server_stats_printer::start() {
  timer_.set_callback([this] {
    ++timer_callback_counter_;
    aggregate_stats().then([this](rpc_server_stats stats) {
      std::stringstream ss;
      ss << stats;
      LOG_INFO("{}", ss.str());
    });
  });
  timer_.arm_periodic(period_);
}

future<> rpc_server_stats_printer::stop() { return make_ready_future<>(); }

future<rpc_server_stats> rpc_server_stats_printer::aggregate_stats() {
  // rpc_server_stats needs to support the + operator,
  // the mapreduce framework is per core!
  return stats_.map_reduce(adder<rpc_server_stats>(), &rpc_server_stats::self);
}

} // end namespace
