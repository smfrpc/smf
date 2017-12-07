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
std::tuple<uint32_t, uint32_t>
put_to_lcore(const char *                            topic,
             const std::size_t &                     topic_size,
             const smf::wal::tx_put_partition_tuple *p) {
  incremental_xxhash32 inc;
  inc.update(topic, topic_size);
  inc.update(p->partition());

  // important to *always* map consistently on this machine
  auto assigned =
    jump_consistent_hash(inc.digest(), std::thread::hardware_concurrency());
  auto runner = jump_consistent_hash(assigned, seastar::smp::count);
  return {assigned, runner};
}

/** \brief map the request to the lcore that is going to handle the put */
uint32_t
get_to_lcore(const smf::wal::tx_get_request *p) {
  incremental_xxhash32 inc;
  inc.update(p->topic()->data(), p->topic()->size());
  inc.update(p->partition());

  // important to *always* map consistently on this machine
  return jump_consistent_hash(inc.digest(),
                              std::thread::hardware_concurrency());
}


std::list<smf::wal_write_request>
core_assignment(const smf::wal::tx_put_request *p) {
  struct assigner {
    assigner(uint32_t runner, uint32_t assigned)
      : runner_core(runner), assigned_core(assigned) {}
    uint32_t           runner_core;
    uint32_t           assigned_core;
    std::set<uint32_t> partitions{};
  };

  if (SMF_UNLIKELY(p->data() == nullptr || p->data()->Length() == 0)) {
    LOG_THROW_IF(p->data() == nullptr, "null pointer to transactions");
    LOG_THROW_IF(p->data()->Length() == 0, "There are no partitions available");
    return {};
  }

  std::vector<assigner> view{};
  view.reserve(p->data()->size());

  for (auto it : *(p->data())) {
    auto[assignment, runner] =
      smf::put_to_lcore(p->topic()->data(), p->topic()->size(), it);

    auto vit =
      std::find_if(view.begin(), view.end(), [assignment, runner](auto &v) {
        return v.assigned_core == assignment && v.runner_core == runner;
      });

    if (vit == view.end()) {
      view.emplace_back(runner, assignment);
    } else {
      vit->partitions.insert(it->partition());
    }
  }
  return std::accumulate(
    view.begin(), view.end(), std::list<smf::wal_write_request>{}, [
      p, prio = smf::priority_manager::thread_local_instance()
                  .streaming_write_priority()
    ](auto acc, const auto &v) {
      acc.push_front(smf::wal_write_request(p, prio, v.assigned_core,
                                            v.runner_core, v.partitions));
      return acc;
    });
}


}  // namespace smf
