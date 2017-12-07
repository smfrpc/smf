// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <core/sstring.hh>
#include <core/timer.hh>

namespace smf {
struct wal_opts {
  explicit wal_opts(
    seastar::sstring           log_directory,
    seastar::timer<>::duration flush_period = std::chrono::minutes(1));
  wal_opts(wal_opts &&o) noexcept;
  wal_opts(const wal_opts &o);

  /// \brief root dir of the WAL
  const seastar::sstring           directory;
  const seastar::timer<>::duration writer_flush_period;
};
std::ostream &operator<<(std::ostream &o, const wal_opts &opts);

}  // namespace smf
