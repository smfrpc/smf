// Copyright 2018 Alexander Gallego
//

#pragma once

#include <vector>

#include <core/aligned_buffer.hh>
#include <core/memory.hh>

#include "platform/log.h"
#include "platform/macros.h"

namespace smf {
static constexpr std::size_t kAllocatorSize = 1 << 18; /*256KB*/
static_assert(kAllocatorSize % 4096 == 0, "invalid alignment for pages");


class page_cache_bump_allocator {
 public:
  explicit page_cache_bump_allocator(std::size_t pgz)
    : page_size(pgz)
    , buf_(seastar::allocate_aligned_buffer<char>(kAllocatorSize, pgz)) {}
  page_cache_bump_allocator(page_cache_bump_allocator &&o) noexcept
    : page_size(o.page_size)
    , buf_(std::move(o.buf_))
    , free_count_(o.free_count_)
    , offset_(o.offset_) {}
  ~page_cache_bump_allocator() = default;

  SMF_DISALLOW_COPY_AND_ASSIGN(page_cache_bump_allocator);

  void
  dealloc(const char *ptr) {
#ifndef NDEBUG
    auto del_t = std::this_thread::get_id();
    LOG_THROW_IF(
      tid_ != del_t,
      "Deleting thread: {} is not the same as the constructor thread: {}",
      del_t, tid_);
#endif
    // verify in debug mode that the pointer belongs to us!
    //
    DLOG_THROW_IF(ptr == nullptr || ptr < data() || ptr > data() + page_size,
                  "Invalid memory region passed: {}", uintptr_t(ptr));
    free_count_ += page_size;
  }

  char *
  alloc() {
    LOG_THROW_IF(is_full(), "allocator is full");
    char *t = data() + offset_;
    offset_ += page_size;
    return t;
  }

  inline bool
  is_empty() const {
    return offset_ == 0;
  }

  inline bool
  is_full() const {
    return offset_ == kAllocatorSize;
  }

  // we have either not allocated anything, or!
  // we've released all the memory already
  //
  inline bool
  is_evictable() const {
    return is_empty() || free_count_ == kAllocatorSize;
  }

  inline void
  evict() {
    offset_     = 0;
    free_count_ = 0;
  }
  const std::size_t page_size;

  inline char *
  data() const {
    return buf_.get();
  }

 private:
  std::unique_ptr<char[], seastar::free_deleter> buf_;
  std::size_t                                    free_count_{0};
  std::size_t                                    offset_{0};

#ifndef NDEBUG
  std::thread::id tid_ = std::this_thread::get_id();
#endif
};

class page_cache_bucket_allocator {
 public:
  SMF_DISALLOW_COPY_AND_ASSIGN(page_cache_bucket_allocator);

  struct page {
    page(char *data, page_cache_bump_allocator *real)
      : data_(DTHROW_IFNULL(data)), bump_(DTHROW_IFNULL(real)) {}

    const char *
    get() const {
      return data_;
    }
    char *
    get() {
      return data_;
    }

    const char *operator->() const { return data_; }

    static void
    dispose(page *p) {
      p->bump_->dealloc(p->get());
    }

   private:
    char *                     data_;
    page_cache_bump_allocator *bump_;
  };
  using page_ptr_t = seastar::lw_shared_ptr<page>;

  page_cache_bucket_allocator(uint64_t min_bytes, uint64_t max_bytes)
    : min_bytes_(min_bytes), max_bytes_(max_bytes) {
    DLOG_THROW_IF(min_bytes_ % 4096 != 0,
                  "incorrect min_bytes for allocator: {}", min_bytes_);
    DLOG_THROW_IF(max_bytes_ % 4096 != 0,
                  "incorrect min_bytes for allocator: {}", max_bytes_);
  }


  uint64_t
  min_bytes() const {
    return min_bytes_;
  }

  uint64_t
  max_bytes() const {
    return max_bytes_;
  }

  void
  set_max_bytes(uint64_t new_max_bytes) {
    max_bytes_ = new_max_bytes;
  }

  page_ptr_t
  alloc(size_t sz) {
    if (4096 % sz != 0) {
      LOG_ERROR("cannot allocate page size: {}", sz);
      throw std::bad_alloc();
    }


    if (used_bytes() >= max_bytes_) { reclaim(); }

    auto get_alloc = [this](auto &vec, std::size_t pg_size) {
      if (vec.empty()) {
        vec.emplace_back(std::make_unique<page_cache_bump_allocator>(pg_size));
      }
      auto a = vec.rbegin()->get();
      if (a->is_full()) {
        if (used_bytes() >= max_bytes_) {
          LOG_ERROR("memory full (even after reclamation) used bytes: {}",
                    used_bytes());
          throw std::bad_alloc();
        }
        vec.emplace_back(std::make_unique<page_cache_bump_allocator>(pg_size));
        a = vec.rbegin()->get();
      };
      return a;
    };


    page_cache_bump_allocator *pa = nullptr;
    switch (sz) {
    case 512:
      pa = get_alloc(alloc512_, 512);
      break;
    case 4096:
      pa = get_alloc(alloc4096_, 4096);
      break;
    default:
      LOG_ERROR("Could not find allocator for alignment of {}", sz);
      throw std::bad_alloc();
    }
    return seastar::make_lw_shared<page>(pa->alloc(), pa);
  }

  inline uint64_t
  used_bytes() {
    return (alloc512_.size() + alloc4096_.size()) * kAllocatorSize;
  }

  void
  reclaim() {
    if (used_bytes() <= max_bytes_) { return; }

    auto reclaim_one = [](auto &vec) {
      for (auto i = 0u; i < vec.size(); ++i) {
        if (vec[i]->is_evictable()) {
          vec[i] = nullptr;
          std::swap(vec[i], vec[vec.size() - 1]);
          vec.pop_back();
          return;
        }
      }
    };
    for (std::size_t i   = 0u,
                     max = std::max(alloc512_.size(), alloc4096_.size());
         i < max && used_bytes() <= max_bytes_; ++i) {
      reclaim_one(alloc512_);
      reclaim_one(alloc4096_);
    }
  }

 private:
  const uint64_t min_bytes_;
  uint64_t       max_bytes_;
  // always alloc<>_.rbegin() points to the last allocator w/ space
  //
  std::vector<std::unique_ptr<page_cache_bump_allocator>> alloc512_{};
  std::vector<std::unique_ptr<page_cache_bump_allocator>> alloc4096_{};
};
}  // namespace smf
