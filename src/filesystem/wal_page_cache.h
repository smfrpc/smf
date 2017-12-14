// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#pragma once

#include <cmath>
#include <list>
#include <utility>

#include <core/aligned_buffer.hh>
#include <core/file.hh>
#include <core/reactor.hh>

#include "filesystem/wal_requests.h"
#include "platform/macros.h"
#include "utils/caching/clock_pro/clock_pro.h"


namespace smf {
struct page_cache_data {
  using bufptr_t = std::unique_ptr<char[], seastar::free_deleter>;
  page_data(uint32_t size, bufptr_t d) : buf_size(size), data(std::move(d)) {}
  page_data(page_data &&d) noexcept
    : buf_size(std::move(d.buf_size))
    , data(std::move(d.data))
    , active_readers(d.active_readers) {}

  const uint32_t buf_size;
  bufptr_t       data;
  SMF_DISALLOW_COPY_AND_ASSIGN(page_cache_data);
};

/// brief - write ahead log
class wal_page_cache {
 public:
  using page_t  = seastar::lw_shared_ptr<page_cache_data>;
  using cache_t = smf::clock_pro_cache<uint64_t, page_t>;


 public:
  ~wal_page_cache() = default;
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_page_cache);

  static wal_page_cache &get() {
    static thread_local wal_page_cache cache;
    return cache;
  }


  future<page_t> cache_read(seastar::lw_shared_ptr<seastar::file> f,
                             // this inode is the static_cast ( stat.ino_t )
                             uint64_t                          inode,
                             uint32_t                          page,
                             const seastar::io_priority_class &pc);


 private:
  wal_page_cache() = default;
  std::unique_ptr<cache_t> cache_;
};

}  // namespace smf
