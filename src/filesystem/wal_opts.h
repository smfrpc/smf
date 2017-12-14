// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <core/sstring.hh>

namespace smf {
struct wal_opts {
  explicit wal_opts(seastar::sstring log_directory);
  wal_opts(wal_opts &&o) noexcept;
  wal_opts(const wal_opts &o);
  wal_opts &operator=(const wal_opts &o);
  /// \brief root dir of the WAL
  const seastar::sstring directory;
};
std::ostream &operator<<(std::ostream &o, const wal_opts &opts);

}  // namespace smf
