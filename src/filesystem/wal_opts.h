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

struct reader_stats {
  reader_stats() {}
  reader_stats(reader_stats &&o) noexcept;
  reader_stats(const reader_stats &o);
  reader_stats &operator+=(const reader_stats &o) noexcept;
  reader_stats &operator=(const reader_stats &o);

  uint64_t                        total_reads{0};
  uint64_t                        total_bytes{0};
  uint64_t                        total_flushes{0};
  std::unique_ptr<smf::histogram> hist = std::make_unique<histogram>();
};
std::ostream &operator<<(std::ostream &o, const reader_stats &s);

struct writer_stats {
  writer_stats() {}
  writer_stats(writer_stats &&o) noexcept;
  writer_stats(const writer_stats &o);
  writer_stats &operator+=(const writer_stats &o) noexcept;
  writer_stats &operator=(const writer_stats &o);

  uint64_t                        total_writes{0};
  uint64_t                        total_bytes{0};
  uint64_t                        total_invalidations{0};
  std::unique_ptr<smf::histogram> hist = std::make_unique<histogram>();
};
std::ostream &operator<<(std::ostream &o, const writer_stats &s);

struct cache_stats {
  uint64_t total_reads{0};
  uint64_t total_writes{0};
  uint64_t total_invalidations{0};
  uint64_t total_bytes_written{0};
  uint64_t total_bytes_read{0};

  cache_stats &operator+=(const cache_stats &o) noexcept;
};
std::ostream &operator<<(std::ostream &o, const cache_stats &s);

// this should probably be a sharded<wal_otps> &
// like the tcp server no?
struct wal_opts {
  explicit wal_opts(seastar::sstring log_directory);
  wal_opts(wal_opts &&o) noexcept;
  wal_opts(const wal_opts &o);
  wal_opts &operator=(const wal_opts &o);

  const seastar::sstring directory;
  const uint64_t         cache_size = wal_file_size_aligned() * 2;
  reader_stats           rstats;
  writer_stats           wstats;
  cache_stats            cstats;
};
std::ostream &operator<<(std::ostream &o, const wal_opts &opts);

}  // namespace smf
