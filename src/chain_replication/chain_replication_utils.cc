// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "chain_replication/chain_replication_utils.h"
#include "hashing_utils.h"

namespace smf {
namespace chains {
/** \brief map the request to the lcore that is going to handle the put */
uint32_t put_to_lcore(const tx_put *p) {
  return jump_consistent_hash(
    xxhash_64(p->topic()->c_str(), p->topic()->size()), smp::count);
}

}  // end namespace chains
}  // end namespace smf
