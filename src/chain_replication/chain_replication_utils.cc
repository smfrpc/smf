// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <limits>
#include <type_traits>

#include <fmt/format.h>

#include "chain_replication/chain_replication_utils.h"
#include "hashing/hashing_utils.h"

namespace smf {
namespace chains {

/** \brief map the request to the lcore that is going to handle the put */
uint32_t put_to_lcore(const char *topic, smf::wal::tx_put_partition_pair *p) {
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


}  // end namespace chains
}  // end namespace smf
