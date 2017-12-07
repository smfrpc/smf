// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
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

  explicit wal_write_behind_cache(seastar::sstring topic_name,
                                  uint32_t         topic_partition,
                                  seastar::file_output_stream_options o);
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
  wal_write_behind_cache_stats stats_;
  uint64_t                     current_size_{0};
  // need fast lookup for offsets
  smf::flat_hash_map<uint64_t, item_ptr> puts_{};
  // ska map is unordered need to keep the next key to evict in order
  std::vector<uint64_t>           keys_{};
  seastar::metrics::metric_groups metrics_{};
};

}  // namespace smf
