// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <limits>
#include <type_traits>

#include <fmt/format.h>

#include "chain_replication/chain_replication_utils.h"
#include "hashing/hashing_utils.h"

namespace smf {
namespace chains {

template <typename T>
requires(std::is_pointer<T>::value == true) uint32_t lcore_hash(T p) {
  fmt::MemoryWriter w;
  w.write("{}:{}", p->topic()->c_str(), p->partition());
  const uint64_t tp_hash = xxhash_64(w.data(), w.size());
  return jump_consistent_hash(tp_hash, seastar::smp::count);
}

/** \brief map the request to the lcore that is going to handle the put */
uint32_t put_to_lcore(const tx_put_request *p) { return lcore_hash(p); }

/** \brief map the request to the lcore that is going to handle the put */
uint32_t get_to_lcore(const tx_get_request *p) { return lcore_hash(p); }


}  // end namespace chains
}  // end namespace smf
