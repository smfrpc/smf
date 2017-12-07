// Copyright 2018 Alexander Gallego
//

#include "filesystem/wal_page_cache.h"

#include <core/shared_ptr.hh>

#include "hashing/hashing_utils.h"
#include "platform/log.h"
#include "platform/macros.h"

namespace smf {

wal_page_cache::wal_page_cache(std::unique_ptr<page_cache_windowing_policy> p,
                               uint64_t min_bytes,
                               uint64_t max_bytes)
  : clock_(min_bytes / 4096 /*some initial number of pages*/)
  , policy_(std::move(p))
  , alloc_(
      std::make_unique<page_cache_bucket_allocator>(min_bytes, max_bytes)) {}


seastar::future<wal_page_cache::page_t>
wal_page_cache::page_fault(seastar::lw_shared_ptr<seastar::file> f,
                           const uint64_t &                      page_idx,
                           const uint32_t &                      page,
                           const seastar::io_priority_class &    pc) {
  const uint64_t alignment         = f->disk_read_dma_alignment();
  const uint64_t page_offset_begin = static_cast<uint64_t>(page) * alignment;

  DLOG_INFO("PAGE FAULT: page alignment: {}, page_offset_begin:{}, page_idx: "
            "{}, page: {}",
            alignment, page_offset_begin, page_idx, page);


  auto predicate = [this, alignment](auto limit) -> bool {
    return (alloc_->used_bytes() + alignment) >= limit;
  };
  // gentle hint! use `min_bytes` and *not* max_bytes
  if (predicate(alloc_->min_bytes())) { clock_.run_hands(); }
  // safety! use `max_bytes`
  for (auto i = 0u; i < clock_.clock_size() && predicate(alloc_->max_bytes());
       ++i) {
    clock_.run_hands();
  }

  auto retval = alloc_->alloc(f->disk_read_dma_alignment());
  auto fut    = f->dma_read(page_offset_begin, retval->get(), alignment, pc);
  return std::move(fut).then(
    [this, page_idx, alignment, retval](auto size) mutable {
      DLOG_INFO("Read: {} bytes", size);
      if (size == 0) {
        return seastar::make_ready_future<page_t>(stdx::nullopt);
      }
      DLOG_THROW_IF(size > alignment, "Read more than 1 page: {}", alignment);
      clock_.add_page(page_idx, static_cast<uint16_t>(size), retval);
      return seastar::make_ready_future<page_t>(clock_.get(page_idx).value());
    });
}


seastar::future<wal_page_cache::page_t>
wal_page_cache::cache_read(seastar::lw_shared_ptr<seastar::file> f,
                           const uint64_t &                      global_fid,
                           const uint32_t &                      original_page,
                           const seastar::io_priority_class &    pc) {
  incremental_xxhash64 inc;
  inc.update(global_fid);
  inc.update(original_page);
  const uint64_t page_idx = inc.digest();

  DLOG_INFO("global_file_id: {}, original_page: {}, compuated page_idx: {}",
            global_fid, original_page, page_idx);


  auto ret = clock_.get(page_idx);
  if (ret) {
    DLOG_INFO("clock-pro says it contains the page index: {}", page_idx);
    return seastar::make_ready_future<page_t>(ret.value());
  }

  return page_fault(f, page_idx, original_page, pc);
}

}  // namespace smf
