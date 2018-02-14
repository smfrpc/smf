// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_lcore_map.h"

#include <limits>
#include <type_traits>

#include <core/reactor.hh>
#include <fmt/format.h>

#include "hashing/hashing_utils.h"

namespace smf {

/** \brief map the request to the lcore that is going to handle the put */
inline uint32_t
put_to_lcore(const char *                            topic,
             const std::size_t &                     topic_size,
             const smf::wal::tx_put_partition_tuple *p) {
  incremental_xxhash32 inc;
  inc.update(topic, topic_size);
  inc.update(p->partition());
  return jump_consistent_hash(inc.digest(), seastar::smp::count);
}

/** \brief map the request to the lcore that is going to handle the put */
inline uint32_t
get_to_lcore(const smf::wal::tx_get_request *p) {
  incremental_xxhash32 inc;
  inc.update(p->topic()->data(), p->topic()->size());
  inc.update(p->partition());

  // important to *always* map consistently on this machine
  return jump_consistent_hash(inc.digest(), seastar::smp::count);
}


std::list<smf::wal_write_request>
core_assignment(const smf::wal::tx_put_request *p) {
  struct assigner {
    explicit assigner(uint32_t runner) : runner_core(runner) {}
    uint32_t                                              runner_core;
    std::vector<const smf::wal::tx_put_partition_tuple *> partitions;
  };

  if (SMF_UNLIKELY(p->data() == nullptr || p->data()->Length() == 0)) {
    LOG_THROW_IF(p->data() == nullptr, "null pointer to transactions");
    LOG_THROW_IF(p->data()->Length() == 0, "There are no partitions available");
    return {};
  }

  std::vector<assigner> view;
  view.reserve(seastar::smp::count);
  for (auto i = 0u; i < seastar::smp::count; ++i) { view.emplace_back(i); }
  for (const smf::wal::tx_put_partition_tuple *it : *(p->data())) {
    auto runner = smf::put_to_lcore(p->topic()->data(), p->topic()->size(), it);
    view[runner].partitions.push_back(it);
  }
  std::list<smf::wal_write_request> retval;
  for (auto &&v : view) {
    retval.push_front(smf::wal_write_request(
      p /*original pointer*/,
      smf::priority_manager::get().streaming_write_priority(), v.runner_core,
      std::move(v.partitions)));
  }
  return retval;
}


}  // namespace smf
