#pragma once
#include <core/fair_queue.hh>

class io_priority_class;

namespace smf {
enum class wal_type : uint8_t { disk_with_memory_cache, memory_only };
enum wal_read_request_flags : uint32_t {
  wrrf_flags_no_decompression = 1,
  wrrf_flags_nearest_offset_up = 1 << 1,
  wrrf_flags_nearest_offset_down = 1 << 2,
};

struct wal_read_request {
  uint64_t offset;
  uint64_t size;
  uint32_t flags;
  const ::io_priority_class &pc;
};
enum wal_write_request_flags : uint32_t {
  wwrf_no_compression = 1,
  wwrf_flush_immediately = 1 << 1,
  wwrf_invalidate_payload = 1 << 2
};
struct wal_write_request {
  wal_write_request(uint32_t _flags,
                    temporary_buffer<char> _data,
                    const ::io_priority_class &_pc)
    : flags(_flags), data(std::move(_data)), pc(_pc) {}


  uint32_t flags;
  temporary_buffer<char> data;
  const ::io_priority_class &pc;
  wal_write_request share_range(size_t i, size_t j) {
    assert(j <= data.size());
    return wal_write_request(flags, data.share(i, j), pc);
  }
};
} // namespace smf
