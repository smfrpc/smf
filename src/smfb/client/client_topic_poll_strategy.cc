// Copyright 2017 Alexander Gallego
//

#pragma once

#include <core/lowres_clock.hh>
#include <fastrange.h>

#include "platform/log.h"

namespace smf {
namespace chains {

client_topic_poll_strategy::client_topic_poll_strategy(
  client_topic_poll_strategy::poll_strategy_start default_start,
  std::vector<smf::wal::wal_watermarkT>           ws)
  : default_start_(default_start), watermarks_(std::move(ws)) {}

uint32_t client_topic_poll_strategy::next_offset() const {
  auto  i = fastrange32(round_robin_idx_++, watermarks_.size());
  auto &p = watermarks_[i];
  if (stats_.find(p.partition) == stats_.end()) {
    switch (default_start_) {
    case poll_strategy_start::latest:
      stats_.insert({p.partition, p.high_watermark});
      break;
    case poll_strategy_start::begining:
      stats_.insert({p.partition, p.low_watermark});
      break;
    case poll_strategy_start::stored:
      LOG_THROW("poll_strategy_start::stored not implemented yet");
    case default:
      LOG_THROW("Unknown default_poll_start strategy: {}",
                static_cast<std::underlying_type<poll_strategy_start>::type>(
                  default_start_));
    }
  }
  return stats_[p.partition].next_offset;
}
void client_topic_poll_strategy::update_partition_stats(
  const uint32_t &                       partition,
  const uint64_t &                       next_offset,
  const uint32_t &                       total_records,
  const seastar::lowres_clock::duration &latency) {
  LOG_THROW_IF(stats_.find(partition) != stats_.end(),
               "Tried to update stats for non existent partition: {}",
               partition);

  if (total_records > 0) {
    auto &s              = stats_[partition];
    s.last_fetch         = seastar::lowres_clock::now();
    s.last_fetch_latency = latency;
    s.next_offset        = next_offset;
  }
}

}  // namespace chains
}  // namespace smf
