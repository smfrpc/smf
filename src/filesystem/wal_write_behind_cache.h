// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <deque>
#include <list>
#include <map>
#include <utility>

#include <core/fstream.hh>
#include <core/metrics_registration.hh>
#include <core/sstring.hh>

#include "adt/flat_hash_map.h"
#include "filesystem/wal_requests.h"
#include "filesystem/wal_write_projection.h"
#include "platform/macros.h"

namespace smf {

class wal_write_behind_cache {
 public:
  struct wal_write_behind_cache_stats {
    uint64_t hits{0};
    uint64_t bytes_read{0};
    uint64_t evicted_puts{0};
    uint64_t bytes_written{0};
  };
  using item_ptr = seastar::lw_shared_ptr<wal_write_projection::item>;
  struct eviction_key {
    eviction_key(uint64_t k, uint64_t sz, item_ptr d)
      : key(k), size(sz), data(d) {}
    eviction_key(eviction_key &&o) noexcept
      : key(o.key), size(o.size), data(std::move(o.data)) {}
    uint64_t key;
    uint64_t size;
    item_ptr data;
  };


  wal_write_behind_cache(seastar::sstring                    topic_name,
                         uint32_t                            topic_partition,
                         seastar::file_output_stream_options o);
  wal_write_behind_cache(wal_write_behind_cache &&) noexcept;
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_write_behind_cache);

  const seastar::file_output_stream_options opts;

  uint64_t min_offset();
  uint64_t max_offset();
  inline bool
  is_offset_in_range(const uint64_t &o) {
    return o >= min_offset() && o < max_offset();
  }


  void                                   put(uint64_t offset, item_ptr data);
  seastar::lw_shared_ptr<wal_read_reply> get(const wal_read_request &req);

 private:
  wal_write_behind_cache_stats    stats_;
  uint64_t                        current_size_{0};
  std::deque<eviction_key>        data_{};
  seastar::metrics::metric_groups metrics_{};
};

}  // namespace smf
