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
// smf
#include "filesystem/wal_opts.h"
#include "filesystem/write_ahead_log.h"
#include "platform/log.h"
#include "platform/macros.h"


namespace smf {
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
    uint64_t bytes_written(0);
  };

  wal_write_behind_cache(seastar::file_output_stream_options o) : opts(o) {}
  const seastar::file_output_stream_options                  opts;

  uint64_t min_offset() {
    if (puts_.empty()) { return 0; }
    return puts_.begin()->offset;
  }
  uint64_t max_offset() {
    if (puts_.empty()) { return 0; }
    return puts_.end()->offset;
  }

  void put(mem_put p) {
    current_size_ += p.fragment->size.size();
    stats_.bytes_written += p.fragment->size();

    // update intrusive indexes
    allocated_.emplace_back(std::move(p));
    puts_.insert(allocated_.back());

    if (current_size_ > opts.preallocation_size) {
      ++stats_.evicted_puts;
      uint64_t virtual_offset = allocated_.front().offset;
      auto     it             = puts_.find(virtual_offset);
      if (it != puts_.end()) { puts_.erase(it); }
      current_size_ -= allocated_.front().data.size();
      allocated_.pop_front();
    }
  }

  wal_read_reply get(const wal_read_request &req) {
    wal_read_reply r;

    auto     offset     = req.req->offset();
    uint32_t bytes_left = req.req->max_bytes();
    while (offset < max_offset() && bytes_left > 0) {
      auto it = puts_.find(offset);
      if (it == puts_.end()) { break; }
      if (bytes_left - it->fragment.size() <= 0) { break; }

      bytes_left -= it->fragment.size();
      offset += it->offset;
      ++stats_.hits;
      stats_.bytes_read += it->fragment.size();
      auto f = std::make_unique<wal::tx_get_fragmentT>();
      // MUST use copy constructor here
      f->hdr = std::make_unique<wal::header>(*(it->fragment->hdr.get()));
      f->fragment.reserve(it->fragment->fragment.size());
      std::copy(it->fragment->fragment.begin(), it->fragment->fragment.end(),
                std::back_inserter(f->fragment));
      r.gets.push_back(std::move(f));
    }
    DLOG_DEBUG_IF(r.empty(), "Request from cache was not found.");
    if (!r.empty()) { r.data->mutate_next_offset(offset); }
    return r;
  }

 private:
  wal_write_behind_cache_stats stats_;
  uint64_t                     current_size_{0};
  std::list<mem_put>           allocated_;
  intrusive_map                puts_;
};
* /
}  // namespace smf
