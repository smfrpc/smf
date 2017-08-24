// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <experimental/optional>
#include <memory>
#include <utility>

#include <boost/filesystem.hpp>
#include <core/temporary_buffer.hh>

#include "filesystem/wal_writer_utils.h"
#include "histogram/histogram.h"

namespace smf {
// struct cache_stats {
//   uint64_t total_reads{0};
//   uint64_t total_writes{0};
//   uint64_t total_bytes_written{0};
//   uint64_t total_bytes_read{0};
// };
struct wal_opts {
  enum class log_type {
    disk_with_memory_cache,
    // for testing only
    memory_only
  };
  explicit wal_opts(seastar::sstring log_directory);
  wal_opts(wal_opts &&o) noexcept;
  wal_opts(const wal_opts &o);
  wal_opts &operator=(const wal_opts &o);

  /// \brief root dir of the WAL
  const seastar::sstring directory;

  uint64_t cache_size = wal_file_size_aligned();
  log_type type       = log_type::disk_with_memory_cache;
};
std::ostream &operator<<(std::ostream &o, const wal_opts &opts);

}  // namespace smf
