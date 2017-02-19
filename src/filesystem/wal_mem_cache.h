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
#include "platform/log.h"
#include "platform/macros.h"

namespace smf {

// TODO(agallego) define a memory cache interface, similar to wal.h
// TODO(agallego) add wal_opts read and write stats here!

class wal_mem_cache {
 private:
  struct mem_put : public boost::intrusive::set_base_hook<>,
                   public boost::intrusive::unordered_set_base_hook<> {
    mem_put(uint64_t _offset, temporary_buffer<char> _data)
      : offset(_offset), data(std::move(_data)) {}
    mem_put(mem_put &&p) noexcept : offset(p.offset), data(std::move(p.data)) {}
    const uint64_t         offset;
    temporary_buffer<char> data;
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


 public:
  wal_mem_cache(uint64_t size, cache_stats *s)
    : max_size(size), cstats_(DTHROW_IFNULL(s)) {}

  /// brief - caches at the virtual offset the buffer
  /// it is different from the absolute buffer, because this is after
  /// `buf` is written to disk, which might be compressed
  future<uint64_t> put(uint64_t virtual_offset, temporary_buffer<char> buf) {
    // add the put
    current_size_ += buf.size();
    allocated_.emplace_back(virtual_offset, std::move(buf));
    // index the put
    puts_.insert(allocated_.back());
    if (current_size_ > max_size) {
      uint64_t remove_offset = allocated_.front().offset;
      return remove(remove_offset).then([this, virtual_offset] {
        current_size_ -= allocated_.front().data.size();
        allocated_.pop_front();
        return make_ready_future<uint64_t>(virtual_offset);
      });
    }
    return make_ready_future<uint64_t>(virtual_offset);
  }

  future<wal_read_reply::maybe> get(uint64_t offset) {
    auto it = puts_.find(offset);
    if (it != puts_.end()) {
      temporary_buffer<char>   tmp(it->data.get(), it->data.size());
      wal_read_reply::fragment f(std::move(tmp));
      wal_read_reply           r(std::move(f));
      return make_ready_future<wal_read_reply::maybe>(std::move(r));
    }
    return make_ready_future<wal_read_reply::maybe>();
  }

  /// brief - invalidates
  future<> remove(uint64_t virtual_offset) {
    auto it = puts_.find(virtual_offset);
    if (it != puts_.end()) {
      puts_.erase(it);
      // this explicitly does not loop through the
      // allocated pages as they autoexpire on additions
      // it would be O(N) cost to remove it from memory
    }
    return make_ready_future<>();
  }

 private:
  const uint64_t     max_size;
  cache_stats *      cstats_;
  uint64_t           current_size_{0};
  std::list<mem_put> allocated_;
  intrusive_map      puts_;
};
}  // namespace smf
