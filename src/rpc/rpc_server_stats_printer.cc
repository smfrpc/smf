// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_server_stats_printer.h"
#include "log.h"

namespace smf {
using duration_t = timer<>::duration;
rpc_server_stats_printer::rpc_server_stats_printer(
  distributed<rpc_server_stats> *stats, duration_t d)
  : stats_(THROW_IFNULL(stats)), period_(d) {}

void rpc_server_stats_printer::start() {
  timer_.set_callback([this] {
    aggregate_stats().then(
      [this](rpc_server_stats stats) { LOG_INFO("{}", stats); });
  });
  timer_.arm_periodic(period_);
}

future<> rpc_server_stats_printer::stop() {
  timer_.cancel();
  return make_ready_future<>();
}

future<rpc_server_stats> rpc_server_stats_printer::aggregate_stats() {
  // rpc_server_stats needs to support the + operator,
  // the mapreduce framework is per core!
  return stats_->map_reduce(adder<rpc_server_stats>(), &rpc_server_stats::self);
}

}  // namespace smf
