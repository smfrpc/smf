// Copyright 2017 Alexander Gallego
//
#include "filesystem/wal_opts.h"

#include <boost/filesystem.hpp>

namespace smf {

wal_opts::wal_opts(seastar::sstring           log,
                   seastar::timer<>::duration flush_period)
  : directory(log), writer_flush_period(flush_period) {}

wal_opts::wal_opts(wal_opts &&o) noexcept
  : directory(std::move(o.directory))
  , writer_flush_period(std::move(o.writer_flush_period)) {}

wal_opts::wal_opts(const wal_opts &o)
  : directory(o.directory), writer_flush_period(o.writer_flush_period) {}

std::ostream &
operator<<(std::ostream &o, const wal_opts &opts) {
  o << "wal_opts{directory=" << opts.directory << ", writer_flush_period="
    << std::chrono::duration_cast<std::chrono::milliseconds>(
         opts.writer_flush_period)
         .count()
    << "}";
  return o;
}
}  // namespace smf
