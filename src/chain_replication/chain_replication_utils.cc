// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <limits>

#include "chain_replication/chain_replication_utils.h"
#include "hashing/hashing_utils.h"

namespace smf {
namespace chains {
/** \brief map the request to the lcore that is going to handle the put */
uint32_t put_to_lcore(const tx_put_request *p) {
  const uint64_t partition = static_cast<uint64_t>(p->partition());
  const uint64_t topic_hash =
    xxhash_64(p->topic()->c_str(), p->topic()->size());

  const uint64_t tp_hash = topic_hash ^ partition;

  return jump_consistent_hash(tp_hash, seastar::smp::count);
}

}  // end namespace chains
}  // end namespace smf
