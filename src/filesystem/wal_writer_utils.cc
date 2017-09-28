// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_writer_utils.h"
#include <chrono>
#include <unistd.h>

#include "platform/log.h"
#include "utils/time_utils.h"

namespace smf {
uint64_t wal_file_size_aligned() {
  static const uint64_t pz = ::sysconf(_SC_PAGESIZE);
  // same as hdfs, see package org.apache.hadoop.fs; - 64MB
  // 67108864
  static const constexpr uint64_t kDefaultFileSize =
    static_cast<uint64_t>(std::numeric_limits<int>::max());
  const uint64_t extra_bytes = kDefaultFileSize % pz;

  // account for page size bigger than 64MB
  if (pz > kDefaultFileSize) { return pz; }
  return kDefaultFileSize - extra_bytes;
}

seastar::sstring wal_file_name(const seastar::sstring &work_dir,
                               const seastar::sstring &prefix,
                               uint64_t                epoch) {
  DLOG_THROW_IF(work_dir[work_dir.size() - 1] == '/',
                "Work dirrectory cannot end in /");
  return work_dir + "/" + prefix + ":" + seastar::to_sstring(epoch) + ".wal";
}

}  // namespace smf
