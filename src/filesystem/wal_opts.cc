// Copyright 2017 Alexander Gallego
//
#include "filesystem/wal_opts.h"

#include <boost/filesystem.hpp>

namespace smf {
static seastar::sstring canonical_dir(const seastar::sstring &directory) {
  return boost::filesystem::canonical(directory.c_str()).string();
}

wal_opts::wal_opts(seastar::sstring log) : directory(canonical_dir(log)) {}

wal_opts::wal_opts(wal_opts &&o) noexcept : directory(std::move(o.directory)) {}

wal_opts::wal_opts(const wal_opts &o) : directory(o.directory) {}

wal_opts &wal_opts::operator=(const wal_opts &o) {
  directory = o.directory;
  return *this;
}

std::ostream &operator<<(std::ostream &o, const wal_opts &opts) {
  o << "wal_opts{directory=" << opts.directory << "}";
  return o;
}
}  // namespace smf
