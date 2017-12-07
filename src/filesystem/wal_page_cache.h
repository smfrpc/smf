// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#pragma once

#include <cmath>
#include <list>
#include <utility>

#include <core/file.hh>
#include <core/reactor.hh>

#include "adt/flat_hash_map.h"
#include "filesystem/clock_pro.h"
#include "filesystem/page_cache_bucket_allocator.h"
#include "filesystem/page_cache_windowing_policy.h"
#include "platform/macros.h"
#include "utils/stdx.h"

namespace smf {


class wal_page_cache {
 public:
  using page_t = stdx::optional<user_page>;
  struct metrics {
    uint64_t hits{0};
    uint64_t page_faults{0};
  };
  /// \brief you are probably using this wrong, if you are instantiating
  /// instances of the page cache.
  /// use the thread_local get() method instead
  ///
  wal_page_cache(std::unique_ptr<page_cache_windowing_policy> policy,
                 uint64_t                                     min_bytes,
                 uint64_t                                     max_bytes);
  ~wal_page_cache() = default;
  wal_page_cache(wal_page_cache &&o) noexcept;

  SMF_DISALLOW_COPY_AND_ASSIGN(wal_page_cache);

  // The idea that the implementation of this cache is needed for all
  // files looking to read, so it is thread local.
  // That is each wal_reader_node.h is suppose to call this internally.
  //
  static wal_page_cache &
  get() {
    static thread_local wal_page_cache cache = []() -> wal_page_cache {

      auto aligner = [](auto x) { return x - (x % 4096); };
      // TODO(agallego) - add seastar::memory::set_reclaim_hook();
      //

      return wal_page_cache(std::make_unique<page_cache_windowing_policy>(),
                            aligner(static_cast<uint64_t>(
                              0.3 * seastar::memory::stats().total_memory())),
                            aligner(static_cast<uint64_t>(
                              0.7 * seastar::memory::stats().total_memory())));

    }();

    return cache;
  }


  seastar::future<page_t> cache_read(seastar::lw_shared_ptr<seastar::file> f,
                                     const uint64_t &global_file_id,
                                     const uint32_t &page,
                                     const seastar::io_priority_class &pc);


  seastar::future<> evict_page(uint64_t inode, uint32_t page);

 private:
  seastar::future<page_t> page_fault(seastar::lw_shared_ptr<seastar::file> src,
                                     const uint64_t &                  page_idx,
                                     const uint32_t &                  page,
                                     const seastar::io_priority_class &pc);

 private:
  clock_pro clock_;

  // grow_by or shrink_by between [min, max]
  std::unique_ptr<page_cache_windowing_policy> policy_;
  // actual memory manager
  std::unique_ptr<page_cache_bucket_allocator> alloc_;

  /// \brief counters for prometheus
  metrics stats_{};
};

}  // namespace smf
