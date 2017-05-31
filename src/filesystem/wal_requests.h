// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <list>
#include <numeric>
#include <utility>
// third party
#include <core/fair_queue.hh>
#include <core/file.hh>
// smf
#include "flatbuffers/wal_generated.h"
#include "platform/log.h"
#include "platform/macros.h"

// class seastar::io_priority_class;

namespace smf {
enum class wal_type : uint8_t {
  wal_type_disk_with_memory_cache,
  wal_type_memory_only
};
enum wal_read_request_flags : uint32_t {
  wrrf_flags_no_decompression = 1,
};

struct wal_read_request {
  wal_read_request(int64_t                             _offset,
                   int64_t                             _size,
                   uint32_t                            _flags,
                   const ::seastar::io_priority_class &priority)
    : offset(_offset), size(_size), flags(_flags), pc(priority) {}

  int64_t                             offset;
  int64_t                             size;
  uint32_t                            flags;
  const ::seastar::io_priority_class &pc;
};


struct wal_read_reply {
  using maybe = std::experimental::optional<wal_read_reply>;
  wal_read_reply() {}
  struct fragment {
    fragment(fbs::wal::wal_header h, seastar::temporary_buffer<char> &&d)
      : hdr(std::move(h)), data(std::move(d)) {
      hdr.mutate_size(data.size());
    }
    explicit fragment(seastar::temporary_buffer<char> &&d)
      : data(std::move(d)) {
      hdr.mutate_size(data.size());
      hdr.mutate_flags(fbs::wal::wal_entry_flags::wal_entry_flags_full_frament);
    }

    fragment(fragment &&f) noexcept : hdr(f.hdr), data(std::move(f.data)) {}

    fbs::wal::wal_header            hdr;
    seastar::temporary_buffer<char> data;
    SMF_DISALLOW_COPY_AND_ASSIGN(fragment);
  };

  explicit wal_read_reply(fragment &&f) {
    fragments.emplace_back(std::move(f));
  }
  wal_read_reply(wal_read_reply &&r) noexcept
    : fragments(std::move(r.fragments)) {}
  void merge(wal_read_reply &&r) {
    fragments.splice(std::end(fragments), std::move(r.fragments));
  }
  uint64_t size() {
    return std::accumulate(fragments.begin(), fragments.end(), uint64_t(0),
                           [](uint64_t acc, const auto &it) {
                             return acc + it.data.size()
                                    + sizeof(fbs::wal::wal_header);
                           });
  }
  inline bool empty() const { return fragments.empty(); }
  std::list<wal_read_reply::fragment> &&move_fragments() {
    return std::move(fragments);
  }
  void emplace_back(std::list<wal_read_reply::fragment> &&fs) {
    this->fragments.splice(std::end(fragments), std::move(fs));
  }
  std::list<wal_read_reply::fragment> fragments{};
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_read_reply);
};

enum wal_write_request_flags : uint32_t {
  wwrf_no_compression     = 1,
  wwrf_flush_immediately  = 1 << 1,
  wwrf_invalidate_payload = 1 << 2
};

struct wal_write_request {
  wal_write_request(uint32_t                            _flags,
                    seastar::temporary_buffer<char> &&  _data,
                    const ::seastar::io_priority_class &_pc)
    : flags(_flags), data(std::move(_data)), pc(_pc) {}

  wal_write_request(wal_write_request &&w) noexcept
    : flags(w.flags), data(std::move(w.data)), pc(w.pc) {}

  wal_write_request share_range(size_t i, size_t len) {
    LOG_THROW_IF(i + len > data.size(), "Out of bounds error");
    return wal_write_request(flags, data.share(i, len), pc);
  }


  uint32_t                        flags;
  seastar::temporary_buffer<char> data;

  const ::seastar::io_priority_class &pc;
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_write_request);
};
}  // namespace smf
