#pragma once
// std
#include <chrono>
#include <unistd.h>
// smf
#include "time_utils.h"

namespace smf {
inline uint64_t wal_file_size_aligned() {
  const static uint64_t pz = ::sysconf(_SC_PAGESIZE);
  // same as hdfs, see package org.apache.hadoop.fs; - 64MB
  // 67108864
  const constexpr static uint64_t kDefaultFileSize = 1 << 26;
  const uint64_t extra_bytes = kDefaultFileSize % pz;

  // account for page size bigger than 64MB
  if(pz > kDefaultFileSize) {
    return pz;
  }
  return kDefaultFileSize - extra_bytes;
}

inline sstring wal_file_name(const sstring &prefix, uint64_t epoch) {
  return prefix + "_" + to_sstring(epoch) + ".wal";
}

} // namespace smf
