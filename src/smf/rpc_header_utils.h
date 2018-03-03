// Copyright 2017 Alexander Gallego
//

#pragma once
#include <xxhash.h>

#include "smf/rpc_generated.h"

namespace smf {

inline static uint32_t
rpc_checksum_payload(const char *payload, uint32_t size) {
  return std::numeric_limits<uint32_t>::max() & XXH64(payload, size, 0);
}

template <typename T>
inline void
checksum_rpc(T &hdr, const char *payload, uint32_t size) {
  hdr.mutate_checksum(rpc_checksum_payload(payload, size));
  hdr.mutate_size(size);
}

}  // namespace smf
