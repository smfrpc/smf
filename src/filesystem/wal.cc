// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal.h"
// std
#include <memory>
#include <utility>
// smf
#include "filesystem/wal_impl_with_cache.h"
#include "platform/log.h"


namespace smf {
std::unique_ptr<wal> wal::make_wal(wal_type type, wal_opts opts) {
  switch (type) {
  case wal_type::wal_type_disk_with_memory_cache:
    return std::make_unique<wal_impl_with_cache>(std::move(opts));
  case wal_type::wal_type_memory_only:
  default:
    // fix later
    LOG_THROW("Unsupported type {}", static_cast<uint8_t>(type));
  }
}
}  // namespace smf
