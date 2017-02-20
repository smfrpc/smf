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
#include "filesystem/wal_pretty_print_utils.h"
#include "hashing/hashing_utils.h"
#include "platform/log.h"

namespace smf {

static uint32_t round_up(int64_t file_size, size_t alignment) {
  uint32_t ret = file_size / alignment;
  if (file_size % alignment != 0) {
    ret += 1;
  }
  return ret;
}

wal_clock_pro_cache::wal_clock_pro_cache(lw_shared_ptr<file> f,
                                         int64_t             size,
                                         uint32_t max_pages_in_memory)
  : file_size(size)
  , number_of_pages(round_up(file_size, f->disk_read_dma_alignment()))
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
      mcold_.emplace_front(std::move(chunk));
      return make_ready_future<ret_type>(mcold_.get_cache_chunk_ptr(page));
    });
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
  const uint32_t &page, const io_priority_class &pc) {
  static const uint64_t kAlignment = file_->disk_read_dma_alignment();
  const uint64_t page_offset_begin = static_cast<uint64_t>(page) * kAlignment;

  auto bufptr = allocate_aligned_buffer<char>(kAlignment, kAlignment);
  auto fut = file_->dma_read(page_offset_begin, bufptr.get(), kAlignment, pc);

  return std::move(fut).then(
    [page, bufptr = std::move(bufptr)](auto size) mutable {
      LOG_THROW_IF(size > kAlignment, "Read more than 1 page");
      cache_chunk c(page, size, std::move(bufptr));
      return make_ready_future<wal_clock_pro_cache::cache_chunk>(std::move(c));
    });
}

static uint64_t copy_page_data(temporary_buffer<char> *                buf,
                               const int64_t &                         offset,
                               const int64_t &                         size,
                               const wal_clock_pro_cache::cache_chunk *chunk,
                               const size_t &alignment) {
  LOG_THROW_IF(chunk->buf_size == 0, "Bad page");
  const int64_t bottom_page_offset = offset > 0 ? offset % alignment : 0;
  const int64_t buffer_offset =
    bottom_page_offset > 0 ? (bottom_page_offset % (chunk->buf_size)) : 0;

  const char *   begin_src = chunk->data.get() + buffer_offset;
  char *         begin_dst = buf->get_write() + (buf->size() - size);
  const uint64_t step_size =
    std::min<int64_t>(chunk->buf_size - buffer_offset, size);
  const char *end_src = begin_src + step_size;

  LOG_THROW_IF(step_size == 0, "Invalid range to copy. step: {}", step_size);
  std::copy(begin_src, end_src, begin_dst);
  return step_size;
}

future<temporary_buffer<char>> wal_clock_pro_cache::read_exactly(
  int64_t offset, int64_t size, const io_priority_class &pc) {
  struct buf_req {
    buf_req(int64_t _offset, int64_t _size, const io_priority_class &p)
      : offset(_offset), size(_size), pc(p) {}

    void update_step(uint64_t step) {
      size -= step;
      offset += step;
    }

    int64_t                  offset;
    int64_t                  size;
    const io_priority_class &pc;
  };
  static const size_t kAlignment = file_->disk_read_dma_alignment();
  auto                buf        = make_lw_shared<temporary_buffer<char>>(size);
  auto                req        = make_lw_shared<buf_req>(offset, size, pc);
  auto                max_pages  = 1 + (req->size / kAlignment);
  LOG_THROW_IF(max_pages > number_of_pages,
               "Request asked to read more pages, than available on disk");
  return repeat([buf, this, req]() mutable {
           auto page = offset_to_page(req->offset, kAlignment);
           return this->clock_pro_get_page(page, req->pc)
             .then([buf, req](auto chunk) {
               if (req->size <= 0) {
                 return stop_iteration::yes;
               }
               auto step = copy_page_data(buf.get(), req->offset, req->size,
                                          chunk, kAlignment);
               req->update_step(step);
               return stop_iteration::no;
             });
         })
    .then([buf] {
      return make_ready_future<temporary_buffer<char>>(buf->share());
    });
}

future<wal_read_reply> wal_clock_pro_cache::read(wal_read_request r) {
  return this->do_read(r).then([](auto ret) {
    wal_read_reply reply;
    reply.emplace_back(ret->move_fragments());
    return make_ready_future<wal_read_reply>(std::move(reply));
  });
}

static void validate_header(const fbs::wal::wal_header &hdr,
                            const int64_t &             file_size) {
  LOG_THROW_IF(hdr.flags() == 0 || hdr.checksum() == 0 || hdr.size() == 0,
               "Could not read header from file");
  LOG_THROW_IF(hdr.size() > file_size,
               "Header asked for more data than file_size. Header:{}", hdr);
}

static fbs::wal::wal_header get_header(const temporary_buffer<char> &buf) {
  static const uint32_t kHeaderSize = sizeof(fbs::wal::wal_header);
  fbs::wal::wal_header  hdr;
  LOG_THROW_IF(buf.size() < kHeaderSize, "Could not read header. Only read",
               buf.size());
  char *hdr_ptr = reinterpret_cast<char *>(&hdr);
  std::copy(buf.get(), buf.get() + kHeaderSize, hdr_ptr);
  return hdr;
}

static void validate_payload(const fbs::wal::wal_header &  hdr,
                             const temporary_buffer<char> &buf) {
  LOG_THROW_IF(buf.size() != hdr.size(), "Could not read header. Only read",
               buf.size());
  uint32_t checksum = xxhash_32(buf.get(), buf.size());
  LOG_THROW_IF(checksum != hdr.checksum(),
               "bad header: {}, missmatching checksums. "
               "expected checksum: {}",
               hdr, checksum);
}

future<lw_shared_ptr<wal_read_reply>> wal_clock_pro_cache::do_read(
  wal_read_request r) {
  using ret_type = lw_shared_ptr<wal_read_reply>;
  LOG_THROW_IF(r.offset + r.size > file_size,
               "Expected normalized offsets. Invalid range."
               "see wal_reader_node.cc?");
  static const uint32_t kHeaderSize = sizeof(fbs::wal::wal_header);

  auto ret = make_lw_shared<wal_read_reply>();
  auto req = make_lw_shared<wal_read_request>(r);
  return repeat([ret, this, req]() mutable {
           if (req->size <= 0) {
             return make_ready_future<stop_iteration>(stop_iteration::yes);
           }
           return this->read_exactly(req->offset, kHeaderSize, req->pc)
             .then([this, ret, req](auto buf) mutable {
               auto hdr = get_header(buf);
               validate_header(hdr, file_size);
               req->offset += kHeaderSize;
               req->size -= kHeaderSize;
               return this->read_exactly(req->offset, hdr.size(), req->pc)
                 .then([ret, hdr, this, req](auto buf) mutable {
                   validate_payload(hdr, buf);
                   req->offset += hdr.size();
                   req->size -= hdr.size();
                   wal_read_reply::fragment f(hdr, std::move(buf));
                   ret->fragments.emplace_back(std::move(f));
                   if (req->size <= 0) {
                     return stop_iteration::yes;
                   }
                   return stop_iteration::no;
                 });
             })
             .handle_exception([req](std::exception_ptr eptr) {
               try {
                 if (eptr) {
                   std::rethrow_exception(eptr);
                 }
               } catch (const std::exception &e) {
                 LOG_ERROR("Caught exception: {}. Offset:{}, size:{}", e.what(),
                           req->offset, req->size);
               }
               return stop_iteration::yes;
             });
         })
    .then([ret] { return make_ready_future<ret_type>(ret); });
}

}  // namespace smf
