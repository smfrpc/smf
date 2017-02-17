// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_clock_pro_cache.h"
// std
#include <algorithm>
#include <utility>
// third party
#include <boost/iterator/counting_iterator.hpp>
// smf
#include "filesystem/file_size_utils.h"
#include "platform/log.h"

namespace smf {

wal_clock_pro_cache::wal_clock_pro_cache(lw_shared_ptr<file> f,
                                         size_t              size,
                                         uint32_t max_pages_in_memory)
  : file_size(size)
  , number_of_pages(std::ceil(file_size / f->disk_read_dma_alignment()))
  , file_(std::move(f)) {
  if (max_pages_in_memory == 0) {
    const auto percent     = uint32_t(number_of_pages * .10);
    const auto min_default = std::min<uint32_t>(10, number_of_pages);
    max_resident_pages_    = std::max<uint32_t>(min_default, percent);
  }
}

future<> wal_clock_pro_cache::close() {
  auto f = file_;  // must hold file until handle closes
  return f->close().finally([f] {});
}

size_t wal_clock_pro_cache::size() const {
  // in c++11 and avobe, size on list is constant, not linear
  return mhot_.size() + mcold_.size();
}

/// intrusive containers::iterators are not default no-throw move
/// constructible, so we have to get the ptr to the data which is lame
///
future<const wal_clock_pro_cache::cache_chunk *>
wal_clock_pro_cache::clock_pro_get_page(uint32_t                 page,
                                        const io_priority_class &pc) {
  using ret_type = const cache_chunk *;
  // first modification. The original algo assumes static working set.
  // we fetch things dynamiclly. so just fill up the buffer before eviction
  // agorithm kicks in. nothing fancy
  //
  // if we just let the clock-pro work w/out pre-allocating, you end up w/ one
  // or 2 pages that you are moving between hot and cold and test
  //
  if (size() < number_of_pages) {
    ++stats_.total_allocation;
    return fetch_page(page, pc).then([this, page](auto chunk) {
      mhot_.emplace_back(std::move(chunk));
      return make_ready_future<ret_type>(mhot_.get_cache_chunk_ptr(page));
    });
  }
  // proper clock-pro starts here.
  if (mhot_.contains(page)) {
    ++stats_.hot_hits;
    // it's a hit, just set the reference bit to 1
    auto h = mhot_.get(page);
    h->ref = true;  // replace w/ the roaring bitmap
    return make_ready_future<ret_type>(mhot_.get_cache_chunk_ptr(page));
  }
  if (mcold_.contains(page)) {
    ++stats_.cold_hits;
    // it's a hit, just set the reference bit to 1
    auto c = mcold_.get(page);
    c->ref = true;
    return make_ready_future<ret_type>(mcold_.get_cache_chunk_ptr(page));
  }
  // it's a miss. relclaim the first met cold page w/ ref bit to 0
  auto maybe_hot = mcold_.hand_cold();

  // Note:
  // next run hand_test() -> no need, we don't use that list
  // This is the main difference of the algorithm.
  // next: reorganize_list() -> no need. self organizing dt.

  // we might need to emplace_front on the mhot_ if the ref == 1
  if (maybe_hot) {
    ++stats_.cold_to_hot;
    mhot_.emplace_front(std::move(maybe_hot.value()));
  }

  auto maybe_cold = mhot_.hand_hot();
  // we might need to emplace_back on the mcold_ if the ref == 0
  if (maybe_cold) {
    ++stats_.hot_to_cold;
    mcold_.emplace_back(std::move(maybe_cold.value()));
  }


  // next add page to list
  return fetch_page(page, pc).then([this, page](auto chunk) {
    ++stats_.total_allocation;
    mcold_.emplace_back(std::move(chunk));
    return make_ready_future<ret_type>(mcold_.get_cache_chunk_ptr(page));
  });
}


future<wal_clock_pro_cache::cache_chunk> wal_clock_pro_cache::fetch_page(
  uint32_t page, const io_priority_class &pc) {
  static const size_t kAlignment = file_->disk_read_dma_alignment();
  return file_->dma_read_exactly<char>(page, kAlignment, pc)
    .then([this, page](temporary_buffer<char> buf) {
      cache_chunk c(page, std::move(buf));
      return make_ready_future<wal_clock_pro_cache::cache_chunk>(std::move(c));
    });
}

bool wal_clock_pro_cache::fill(wal_read_reply *                        out,
                               const wal_read_request &                r,
                               const wal_clock_pro_cache::cache_chunk *chunk) {
  static const auto kHeaderSize = sizeof(fbs::wal::wal_header);
  static const auto kAlignment  = file_->disk_read_dma_alignment();
  // bounds
  auto       begin = r.offset % kAlignment;
  auto const end   = begin + std::min(r.size - out->size(), chunk->data.size());
  auto       begin_it_fn = [&]() { return chunk->data.get() + begin; };
  // check if we are not at the right page
  if (chunk->page != offset_to_page(r.offset + out->size(), kAlignment)) {
    return false;
  }
  while (begin < end) {
    if (begin + kHeaderSize > end) {
      // ensure space for copying header from page
      return false;
    }
    // get the header
    fbs::wal::wal_header hdr;
    auto                 hdr_ptr = reinterpret_cast<char *>(&hdr);
    std::copy(begin_it_fn(), begin_it_fn() + kHeaderSize, hdr_ptr);
    // update for header and check again if size
    begin += kHeaderSize;
    if (begin + hdr.size() > end) {
      // no space for body
      return false;
    }
    temporary_buffer<char> buf(hdr.size());
    std::copy(begin_it_fn(), begin_it_fn() + hdr.size(), buf.get_write());
    begin += hdr.size();
    wal_read_reply::fragment f(hdr, std::move(buf));
    out->fragments.emplace_back(std::move(f));
  }
  return true;
}

future<wal_read_reply> wal_clock_pro_cache::read(wal_read_request r) {
  LOG_THROW_IF(r.offset + r.size > file_size,
               "Expected normalized offsets. Invalid range."
               "see wal_reader_node.cc?");
  static const auto kAlignment      = file_->disk_read_dma_alignment();
  const uint64_t    number_of_pages = r.size / kAlignment;
  auto              ret             = make_lw_shared<wal_read_reply>();
  return do_for_each(boost::counting_iterator<int>(0),
                     boost::counting_iterator<int>(number_of_pages),
                     [this, r, ret](int page_idx) mutable {
                       auto next_offset = r.offset + (page_idx * kAlignment);
                       auto page = offset_to_page(next_offset, kAlignment);
                       return clock_pro_get_page(page, r.pc)
                         .then([this, r, ret](const cache_chunk *ptr) mutable {
                           this->fill(ret.get(), r, ptr);
                           return make_ready_future<>();
                         });
                     })
    .then([ret] {
      wal_read_reply reply;
      reply.fragments = std::move(ret->fragments);
      return make_ready_future<wal_read_reply>(std::move(reply));
    });
}
}  // namespace smf
