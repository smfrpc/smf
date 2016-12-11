#pragma once
// std
#include <ostream>
// seastar
#include <core/future.hh>
#include <core/sstring.hh>
// smf:filesystem
#include "filesystem/wal_opts.h"

namespace smf {
enum class wal_type : uint8_t { disk_with_memory_cache, memory_only };

/// brief - write ahead log
class wal {
  public:
  explicit wal(wal_opts o) : opts(std::move(o)) {}
  virtual ~wal() {}
  /// brief - used by seastar map-reduce
  inline const wal_opts &get_otps() const { return opts; }
  const wal_opts opts;
  virtual future<> append(temporary_buffer<char> &&buf) = 0;
  virtual future<> invalidate(uint64_t epoch) = 0;
  virtual future<wal_opts::maybe_buffer> get(uint64_t epoch) = 0;
  static std::unique_ptr<wal> make_wal(wal_type type, wal_opts opts);
};
} // namespace smf
namespace std {
ostream &operator<<(std::ostream &os, smf::wal_type t) {
  switch(t) {
  case smf::wal_type::disk_with_memory_cache:
    os << "smf::wal_type::disk_with_memory_cache";
    break;
  case smf::wal_type::memory_only:
    os << "smf::wal_type::memory_only";
    break;
  default:
    os << "Error. Unknown smf::wal_type. ";
    break;
  }
  return os;
}

} // namespace std
