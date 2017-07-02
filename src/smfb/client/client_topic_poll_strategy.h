// Copyright 2017 Alexander Gallego
//

#pragma once

#include <unordered_map>
#include <vector>

#include <core/lowres_clock.hh>

#include "flatbuffers/chain_replication.smf.fb.h"

namespace smf {
namespace chains {

enum class poll_strategy_start { latest, begining, stored };

class client_topic_poll_strategy {
 public:
  client_topic_poll_strategy(poll_strategy_start default_start,
                             std::vector<smf::wal::wal_watermarkT> ws);


  uint32_t next_offset() const;

  void update_partition_stats(const uint32_t &partition,
                              const uint64_t &next_offset,
                              const uint32_t &total_records,
                              const seastar::lowres_clock::duration &latency);

 private:
  struct client_poll_stats {
    client_poll_stats(uint32_t p, uint64_t o) : partition(p), next_offset(o) {}
    uint32_t                          partition;
    uint64_t                          next_offset;
    seastar::lowres_clock::time_point last_fetch;
    seastar::lowres_clock::duration   last_fetch_latency;
  };

 private:
  poll_strategy_start                   default_start_;
  std::vector<smf::wal::wal_watermarkT> watermarks_;
  std::unordered_map<uint32_t, client_poll_stats> stats_{};
  uint64_t round_robin_idx_{0};
};

}  // namespace chains
}  // namespace smf
