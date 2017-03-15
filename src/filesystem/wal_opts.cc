#include "filesystem/wal_opts.h"

#include "utils/human_bytes_printing_utils.h"

namespace smf {

reader_stats::reader_stats(reader_stats &&o) noexcept
  : total_reads(o.total_reads),
    total_bytes(o.total_bytes),
    total_flushes(o.total_flushes),
    hist(std::move(o.hist)) {}

reader_stats::reader_stats(const reader_stats &o)
  : total_reads(o.total_reads)
  , total_bytes(o.total_bytes)
  , total_flushes(o.total_flushes) {
  hist = std::make_unique<histogram>();
  *hist += *o.hist;
}

reader_stats &reader_stats::operator+=(const reader_stats &o) noexcept {
  total_reads += o.total_reads;
  total_bytes += o.total_bytes;
  total_flushes += o.total_flushes;
  *hist += *o.hist;
  return *this;
}
reader_stats &reader_stats::operator=(const reader_stats &o) {
  reader_stats r(o);
  std::swap(r, *this);
  return *this;
}

std::ostream &operator<<(std::ostream &o, const reader_stats &s) {
  o << "reader_stats{total_reads=" << s.total_reads
    << ",total_bytes=" << s.total_bytes << ",total_flushes=" << s.total_flushes
    << ",hist=" << *s.hist.get() << "}";
  return o;
}


writer_stats::writer_stats(writer_stats &&o) noexcept
  : total_writes(o.total_writes),
    total_bytes(o.total_bytes),
    total_invalidations(o.total_invalidations),
    hist(std::move(o.hist)) {}
writer_stats::writer_stats(const writer_stats &o)
  : total_writes(o.total_writes)
  , total_bytes(o.total_bytes)
  , total_invalidations(o.total_invalidations) {
  hist = std::make_unique<histogram>();
  *hist += *o.hist;
}
writer_stats &writer_stats::operator+=(const writer_stats &o) noexcept {
  total_writes += o.total_writes;
  total_bytes += o.total_bytes;
  total_invalidations += o.total_invalidations;
  *hist += *o.hist;
  return *this;
}
writer_stats &writer_stats::operator=(const writer_stats &o) {
  writer_stats w(o);
  std::swap(w, *this);
  return *this;
}
std::ostream &operator<<(std::ostream &o, const writer_stats &s) {
  o << "writer_stats{total_writes=" << s.total_writes
    << ",total_bytes=" << s.total_bytes
    << ",total_invalidations=" << s.total_invalidations
    << ",hist=" << *s.hist.get() << "}";
  return o;
}

cache_stats &cache_stats::operator+=(const cache_stats &o) noexcept {
  total_reads += o.total_reads;
  total_writes += o.total_writes;
  total_invalidations += o.total_invalidations;
  total_bytes_written += o.total_bytes_written;
  total_bytes_read += o.total_bytes_read;

  return *this;
}
std::ostream &operator<<(std::ostream &o, const cache_stats &s) {
  o << "cache_stats{total_reads=" << s.total_reads
    << ",total_writes=" << s.total_writes
    << ",total_invalidations=" << s.total_invalidations
    << ",total_bytes_writen=" << s.total_bytes_written
    << ",total_bytes_read=" << s.total_bytes_read << "}";
  return o;
}

static sstring canonical_dir(const sstring &directory) {
  return boost::filesystem::canonical(directory.c_str()).string();
}
wal_opts::wal_opts(sstring log_directory) : directory(log_directory) {}
wal_opts::wal_opts(wal_opts &&o) noexcept : directory(std::move(o.directory)),
                                            cache_size(o.cache_size),
                                            rstats(std::move(o.rstats)),
                                            wstats(std::move(o.wstats)),
                                            cstats(std::move(o.cstats)) {}
wal_opts::wal_opts(const wal_opts &o)
  : directory(canonical_dir(o.directory))
  , cache_size(o.cache_size)
  , rstats(o.rstats)
  , wstats(o.wstats)
  , cstats(o.cstats) {}
wal_opts &wal_opts::operator=(const wal_opts &o) {
  wal_opts wo(o);
  std::swap(wo, *this);
  return *this;
}

std::ostream &operator<<(std::ostream &o, const wal_opts &opts) {
  o << "cache_stats{directory=" << opts.directory << ",cache_size=";
  human_bytes(o, opts.cache_size) << ",rstats=" << opts.rstats
                                  << ",wstats=" << opts.wstats
                                  << ",cstats=" << opts.cstats << "}";
  return o;
}
}  // namespace smf
