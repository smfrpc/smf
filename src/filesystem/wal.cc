#include "filesystem/wal.h"
#include "log.h"
#include "filesystem/wal_impl_with_cache.h"
namespace smf {
std::unique_ptr<wal> wal::make_wal(wal_type type, wal_opts opts) {
  switch(type) {
  case wal_type::disk_with_memory_cache:
    return std::make_unique<wal_impl_with_cache>(std::move(opts));
  case wal_type::memory_only:
  default:
    // fix later
    LOG_THROW("Unsupported type {}", static_cast<uint8_t>(type));
  }
}
} // namespace smf
