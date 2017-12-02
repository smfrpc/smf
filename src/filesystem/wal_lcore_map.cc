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
uint32_t put_to_lcore(const char *                           topic,
                      const smf::wal::tx_put_partition_pair *p) {
  fmt::MemoryWriter w;
  w.write("{}:{}", topic, p->partition());
  const uint64_t tp_hash = xxhash_64(w.data(), w.size());
  return jump_consistent_hash(tp_hash, seastar::smp::count);
}

/** \brief map the request to the lcore that is going to handle the put */
uint32_t get_to_lcore(const smf::wal::tx_get_request *p) {
  fmt::MemoryWriter w;
  w.write("{}:{}", p->topic()->c_str(), p->partition());
  const uint64_t tp_hash = xxhash_64(w.data(), w.size());
  return jump_consistent_hash(tp_hash, seastar::smp::count);
}

}  // namespace smf
