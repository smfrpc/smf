#pragma once
// std
#include <memory>
#include <experimental/optional>
// seastar
#include <core/temporary_buffer.hh>
// smf
#include "filesystem/wal_writer_utils.h"
#include "histogram.h"
namespace smf {

struct reader_stats {
  reader_stats() {}
  reader_stats(reader_stats &&o) noexcept : total_reads(o.total_reads),
                                            total_bytes(o.total_bytes),
                                            total_flushes(o.total_flushes),
                                            hist(std::move(o.hist)) {}
  reader_stats(const reader_stats &o) noexcept
    : total_reads(o.total_reads),
      total_bytes(o.total_bytes),
      total_flushes(o.total_flushes) {
    hist = std::make_unique<histogram>();
    *hist += *o.hist;
  }
  reader_stats &operator+=(const reader_stats &o) noexcept {
    total_reads += o.total_reads;
    total_bytes += o.total_bytes;
    total_flushes += o.total_flushes;
    *hist += *o.hist;
    return *this;
  }
  reader_stats &operator=(const reader_stats &o) noexcept {
    reader_stats r(o);
    std::swap(r, *this);
    return *this;
  }

  uint64_t total_reads{0};
  uint64_t total_bytes{0};
  uint64_t total_flushes{0};
  std::unique_ptr<smf::histogram> hist = std::make_unique<histogram>();
};

struct writer_stats {
  writer_stats() {}
  writer_stats(writer_stats &&o) noexcept
    : total_reads(o.total_reads),
      total_bytes(o.total_bytes),
      total_invalidations(o.total_invalidations),
      hist(std::move(o.hist)) {}
  writer_stats(const writer_stats &o) noexcept
    : total_reads(o.total_reads),
      total_bytes(o.total_bytes),
      total_invalidations(o.total_invalidations) {
    hist = std::make_unique<histogram>();
    *hist += *o.hist;
  }
  writer_stats &operator+=(const writer_stats &o) noexcept {
    total_reads += o.total_reads;
    total_bytes += o.total_bytes;
    total_invalidations += o.total_invalidations;
    *hist += *o.hist;
    return *this;
  }
  writer_stats &operator=(const writer_stats &o) noexcept {
    writer_stats w(o);
    std::swap(w, *this);
    return *this;
  }
  uint64_t total_reads{0};
  uint64_t total_bytes{0};
  uint64_t total_invalidations{0};
  std::unique_ptr<smf::histogram> hist = std::make_unique<histogram>();
};

// this should probably be a sharded<wal_otps> &
// like the tcp server no?
struct wal_opts {
  using maybe_buffer = std::experimental::optional<temporary_buffer<char>>;

  explicit wal_opts(sstring log_directory) : directory(log_directory) {}
  wal_opts(wal_opts &&o) noexcept : directory(std::move(o.directory)),
                                    cache_size(o.cache_size),
                                    rstats(std::move(o.rstats)),
                                    wstats(std::move(o.wstats)) {}
  wal_opts(const wal_opts &o) noexcept : directory(o.directory),
                                         cache_size(o.cache_size),
                                         rstats(o.rstats),
                                         wstats(o.wstats) {}
  wal_opts &operator=(const wal_opts &o) noexcept {
    wal_opts wo(o);
    std::swap(wo, *this);
    return *this;
  }
  const sstring directory;
  const uint64_t cache_size = wal_file_size_aligned();
  reader_stats rstats;
  writer_stats wstats;
};

} // namespace smf
