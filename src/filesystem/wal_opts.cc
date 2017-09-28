// Copyright 2017 Alexander Gallego
//
#include "filesystem/wal_opts.h"

#include <boost/filesystem.hpp>


namespace smf {
static seastar::sstring canonical_dir(const seastar::sstring &directory) {
  return boost::filesystem::canonical(directory.c_str()).string();
}

wal_opts::wal_opts(seastar::sstring log) : directory(canonical_dir(log)) {}

wal_opts::wal_opts(wal_opts &&o) noexcept
  : directory(std::move(o.directory)), type(std::move(o.type)) {}

wal_opts::wal_opts(const wal_opts &o) : directory(o.directory), type(o.type) {}

wal_opts &wal_opts::operator=(const wal_opts &o) {
  wal_opts wo(o);
  std::swap(wo, *this);
  return *this;
}

std::ostream &operator<<(std::ostream &o, const wal_opts &opts) {
  o << "wal_opts{directory=" << opts.directory << ",log_type="
    << static_cast<std::underlying_type<wal_opts::log_type>::type>(opts.type)
    << "}";
  return o;
}
}  // namespace smf
