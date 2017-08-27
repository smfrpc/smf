// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <list>
#include <unordered_map>
#include <utility>
// third party
#include <boost/intrusive/options.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <core/metrics_registration.hh>
// smf
#include "filesystem/wal_requests.h"
#include "filesystem/wal_write_projection.h"
#include "platform/macros.h"

namespace smf {

// We keep 1MB of write behind data before flushing to disk. SO we
// MUST take this into account before we yield true. i.e.: we have to keep 1MB
// of data in memory so you can have deterministic readers.
class wal_write_behind_cache {
 public:
  struct mem_put : public boost::intrusive::set_base_hook<>,
                   public boost::intrusive::unordered_set_base_hook<> {
    mem_put(uint64_t                                           _offset,
            seastar::lw_shared_ptr<wal_write_projection::item> data)
      : offset(_offset), fragment(std::move(data)) {}

    mem_put(mem_put &&p) noexcept
      : offset(p.offset), fragment(std::move(p.fragment)) {}

    const uint64_t                                     offset;
    seastar::lw_shared_ptr<wal_write_projection::item> fragment;

    SMF_DISALLOW_COPY_AND_ASSIGN(mem_put);
  };
  struct mem_put_key {
    typedef uint64_t type;
    const type &operator()(const mem_put &v) const { return v.offset; }
  };
  using intrusive_key = boost::intrusive::key_of_value<mem_put_key>;
  using intrusive_map = boost::intrusive::set<mem_put, intrusive_key>;
  static_assert(std::is_same<intrusive_map::key_type, uint64_t>::value,
                "bad key for intrusive map");

  struct wal_write_behind_cache_stats {
    uint64_t hits{0};
    uint64_t bytes_read{0};
    uint64_t evicted_puts{0};
    uint64_t bytes_written{0};
  };

  wal_write_behind_cache(seastar::file_output_stream_options o);
  const seastar::file_output_stream_options opts;

  uint64_t min_offset();
  uint64_t max_offset();
  void put(mem_put p);
  wal_read_reply get(const wal_read_request &req);

 private:
  wal_write_behind_cache_stats    stats_;
  uint64_t                        current_size_{0};
  std::list<mem_put>              allocated_;
  intrusive_map                   puts_;
  seastar::metrics::metric_groups metrics_{};
};

}  // namespace smf
