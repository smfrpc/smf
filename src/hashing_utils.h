// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <boost/crc.hpp>
#include <xxhash.h>
namespace smf {
inline uint32_t crc_32(const char *data, const size_t &length) {
  boost::crc_32_type result;
  result.process_bytes(data, length);
  return result.checksum();
}
inline uint64_t xxhash_64(const char *data, const size_t &length) {
  return XXH64(data, length, 0);
}
inline uint32_t xxhash_32(const char *data, const size_t &length) {
  return XXH32(data, length, 0);
}

/// google's jump consistent hash
/// https://arxiv.org/pdf/1406.2294.pdf
// TODO(agallego) - move this to it's own impl & header
__attribute__((unused)) static uint32_t jump_consistent_hash(
  uint64_t key, uint32_t num_buckets) {
  int64_t b = -1, j = 0;
  while (j < num_buckets) {
    b   = j;
    key = key * 2862933555777941757ULL + 1;
    j   = (b + 1) * (static_cast<double>(1LL << 31)
                   / static_cast<double>((key >> 33) + 1));
  }
  return static_cast<uint32_t>(b);
}


}  // namespace smf
