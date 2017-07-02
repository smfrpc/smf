// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/write_ahead_log.h"
// std
#include <memory>
#include <utility>
// smf
#include "filesystem/wal_impl_with_cache.h"
#include "platform/log.h"


namespace smf {
std::unique_ptr<write_ahead_log> make_unique_wal(wal_opts opts) {
  switch (opts.type) {
  case wal_opts::log_type::disk_with_memory_cache:
    return std::make_unique<wal_impl_with_cache>(std::move(opts));
  default:
    // fix later
    LOG_THROW("Unsupported write_ahead_log type in wal_opts: {}", opts);
  }
}
}  // namespace smf
