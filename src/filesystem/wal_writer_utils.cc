// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_writer_utils.h"
#include <chrono>
#include <unistd.h>

#include "utils/time_utils.h"

namespace smf {
uint64_t wal_file_size_aligned() {
  static const uint64_t pz = ::sysconf(_SC_PAGESIZE);
  // same as hdfs, see package org.apache.hadoop.fs; - 64MB
  // 67108864
  static const constexpr uint64_t kDefaultFileSize = 1 << 26;
  const uint64_t                  extra_bytes      = kDefaultFileSize % pz;

  // account for page size bigger than 64MB
  if (pz > kDefaultFileSize) {
    return pz;
  }
  return kDefaultFileSize - extra_bytes;
}

sstring wal_file_name(const sstring &prefix, uint64_t epoch) {
  return prefix + ":" + to_sstring(epoch) + ".wal";
}

}  // namespace smf
