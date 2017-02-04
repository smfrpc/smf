// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <list>
#include <utility>
// third party
#include <core/fair_queue.hh>
// generated
#include "flatbuffers/wal_generated.h"

class io_priority_class;

namespace smf {
enum class wal_type : uint8_t {
  wal_type_disk_with_memory_cache,
  wal_type_memory_only
};
enum wal_read_request_flags : uint32_t {
  wrrf_flags_no_decompression    = 1,
  wrrf_flags_nearest_offset_up   = 1 << 1,
  wrrf_flags_nearest_offset_down = 1 << 2,
};

struct wal_read_request {
  uint64_t offset;
  uint64_t size;
  uint32_t flags;

  const ::io_priority_class &pc;
};


struct wal_read_reply {
  using maybe = std::experimental::optional<wal_read_reply>;

  struct fragment {
    fragment(fbs::wal::wal_header h, temporary_buffer<char> d)
      : hdr(std::move(h)), data(std::move(d)) {}
    explicit fragment(temporary_buffer<char> d) : data(std::move(d)) {
      hdr.mutate_size(data.size());
      hdr.mutate_flags(fbs::wal::wal_entry_flags::wal_entry_flags_full_frament);
    }


    fbs::wal::wal_header   hdr;
    temporary_buffer<char> data;
  };

  wal_read_reply() = default;
  // forward ctor
  explicit wal_read_reply(temporary_buffer<char> d)
    : wal_read_reply(fragment(std::move(d))) {}

  explicit wal_read_reply(fragment &&f) { fragments.push_back(std::move(f)); }

  std::list<wal_read_reply::fragment> fragments{};
};

enum wal_write_request_flags : uint32_t {
  wwrf_no_compression     = 1,
  wwrf_flush_immediately  = 1 << 1,
  wwrf_invalidate_payload = 1 << 2
};

struct wal_write_request {
  wal_write_request(uint32_t                   _flags,
                    temporary_buffer<char>     _data,
                    const ::io_priority_class &_pc)
    : flags(_flags), data(std::move(_data)), pc(_pc) {}


  wal_write_request share_range(size_t i, size_t j) {
    assert(j <= data.size());
    return wal_write_request(flags, data.share(i, j), pc);
  }


  uint32_t               flags;
  temporary_buffer<char> data;

  const ::io_priority_class &pc;
};
}  // namespace smf
