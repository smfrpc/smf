// Copyright 2017 Alexander Gallego
//

#pragma once

#include "hashing/hashing_utils.h"
#include "flatbuffers/rpc_generated.h"

namespace smf {
template <typename T>
inline void checksum_rpc(T &hdr, const char *payload, uint32_t size) {
  hdr.mutate_checksum(xxhash_32(payload, size));
  hdr.mutate_size(size);
}
}  // namespace smf
