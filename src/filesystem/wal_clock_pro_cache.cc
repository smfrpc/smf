// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_clock_pro_cache.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include <boost/iterator/counting_iterator.hpp>

#include "filesystem/file_size_utils.h"
#include "filesystem/wal_pretty_print_utils.h"
#include "hashing/hashing_utils.h"
#include "platform/log.h"

namespace smf {

static uint32_t round_up(int64_t file_size, size_t alignment) {
  uint32_t ret = file_size / alignment;
  if (file_size % alignment != 0) { ret += 1; }
  return ret;
}

wal_clock_pro_cache::wal_clock_pro_cache(
  seastar::lw_shared_ptr<seastar::file> f,
  int64_t                               initial_size,
  uint32_t                              max_pages_in_memory)
  : disk_dma_alignment(f->disk_read_dma_alignment())
  , file_size_(initial_size)
  , number_of_pages_(round_up(initial_size, disk_dma_alignment))
  , file_(std::move(f)) {
  using cache_t = smf::clock_pro_cache<uint64_t, page_data>;
  cache_        = std::make_unique<cache_t>(max_resident_pages_);
}
void wal_clock_pro_cache::update_file_size_by(uint64_t delta) {
  file_size_ += delta;
  number_of_pages_ = round_up(file_size_, disk_dma_alignment);
}

seastar::future<> wal_clock_pro_cache::close() {
  auto f = file_;  // must hold file until handle closes
  return f->close().finally([f] {});
}

// this function needs testing
static uint64_t copy_page_data(seastar::temporary_buffer<char> *      buf,
                               const int64_t &                        offset,
                               const int64_t &                        size,
                               const wal_clock_pro_cache::chunk_t_ptr chunk,
                               const size_t &alignment) {
  LOG_THROW_IF(chunk->data.buf_size == 0, "Bad page");
  const int64_t bottom_page_offset = offset > 0 ? offset % alignment : 0;
  const int64_t buffer_offset =
    bottom_page_offset > 0 ? (bottom_page_offset % (chunk->data.buf_size)) : 0;

  const char *   begin_src = chunk->data.data.get() + buffer_offset;
  char *         begin_dst = buf->get_write() + (buf->size() - size);
  const uint64_t step_size =
    std::min<int64_t>(chunk->data.buf_size - buffer_offset, size);
  const char *end_src = begin_src + step_size;

  LOG_THROW_IF(step_size == 0, "Invalid range to copy. step: {}", step_size);
  std::copy(begin_src, end_src, begin_dst);
  return step_size;
}

seastar::future<seastar::temporary_buffer<char>>
wal_clock_pro_cache::read_exactly(int64_t                           offset,
                                  int64_t                           size,
                                  const seastar::io_priority_class &pc) {
  struct buf_req {
    buf_req(int64_t _offset, int64_t _size, const seastar::io_priority_class &p)
      : offset(_offset), size(_size), pc(p) {}

    void update_step(uint64_t step) {
      size -= step;
      offset += step;
    }

    int64_t                           offset;
    int64_t                           size;
    const seastar::io_priority_class &pc;
  };
  static const size_t disk_dma_alignment = disk_dma_alignment;
  auto buf = seastar::make_lw_shared<seastar::temporary_buffer<char>>(size);
  auto req = seastar::make_lw_shared<buf_req>(offset, size, pc);
  auto max_pages = 1 + (req->size / disk_dma_alignment);
  LOG_THROW_IF(max_pages > number_of_pages_,
               "Request asked to read more pages, than available on disk");
  return seastar::repeat([buf, this, req]() mutable {
           auto page = offset_to_page(req->offset, disk_dma_alignment);
           return this->clock_pro_get_page(page, req->pc)
             .then([buf, req](auto chunk) {
               if (req->size <= 0) { return seastar::stop_iteration::yes; }
               auto step = copy_page_data(buf.get(), req->offset, req->size,
                                          chunk, disk_dma_alignment);
               req->update_step(step);
               return seastar::stop_iteration::no;
             });
         })
    .then([buf] {
      return seastar::make_ready_future<seastar::temporary_buffer<char>>(
        buf->share());
    });
}

static void debug_only_validate_header(const wal::wal_header &hdr,
                                       const int64_t &        file_size) {
  DLOG_THROW_IF(hdr.checksum() == 0 || hdr.size() == 0,
                "Could not read header from file");
  DLOG_THROW_IF(hdr.size() > file_size,
                "Header asked for more data than file_size. Header:{}", hdr);
}

static wal::wal_header get_header(const seastar::temporary_buffer<char> &buf) {
  static const uint32_t kHeaderSize = sizeof(wal::wal_header);
  wal::wal_header       hdr;
  LOG_THROW_IF(buf.size() < kHeaderSize, "Could not read header. Only read",
               buf.size());
  char *hdr_ptr = reinterpret_cast<char *>(&hdr);
  std::copy(buf.get(), buf.get() + kHeaderSize, hdr_ptr);
  return hdr;
}

static void debug_only_validate_payload(
  const wal::wal_header &hdr, const seastar::temporary_buffer<char> &buf) {
  DLOG_THROW_IF(buf.size() != hdr.size(),
                "Could not read header. Only read {} bytes", buf.size());
  DLOG_THROW_IF(xxhash_32(buf.get(), buf.size()) != hdr.checksum(),
                "bad header: {}, missmatching checksums. "
                "expected checksum: {}",
                hdr, xxhash_32(buf.get(), buf.size()));
}

seastar::future<wal_read_reply> wal_clock_pro_cache::read(
  wal_read_request r, uint64_t relative_offset) {
  wal_read_reply retval;
  LOG_THROW_IF(r.adjusted_offset() > static_cast<uint64_t>(file_size_),
               "Invalid offset range for file");
  static const uint32_t kHeaderSize = sizeof(wal::wal_header);
  auto logerr                       = [r, retval](auto eptr) {
    LOG_ERROR("Caught exception: {}. Request: {}, Reply:{}", eptr, r, retval);
    return seastar::stop_iteration::yes;
  };
  return seastar::repeat([r, this, retval, logerr, relative_offset]() mutable {
           if (retval.size() >= r.req->max_bytes()) {
             return seastar::make_ready_future<seastar::stop_iteration>(
               seastar::stop_iteration::yes);
           }

           uint64_t next_offset =
             r.adjusted_offset() + retval.size() - relative_offset;

           if (next_offset >= static_cast<uint64_t>(file_size())) {
             return seastar::make_ready_future<seastar::stop_iteration>(
               seastar::stop_iteration::yes);
           }

           return this->read_exactly(next_offset, kHeaderSize, r.pc)
             .then([this, r, retval, next_offset, logerr](auto buf) mutable {
               auto hdr = get_header(buf);
               next_offset += kHeaderSize;
               debug_only_validate_header(hdr, file_size_);
               return this->read_exactly(next_offset, hdr.size(), r.pc)
                 .then([retval, hdr, this, r, logerr](auto buf) mutable {
                   debug_only_validate_payload(hdr, buf);
                   auto ptr = std::make_unique<wal::tx_get_fragmentT>();
                   ptr->hdr = std::make_unique<wal::wal_header>(std::move(hdr));
                   // copy to the std::vector
                   ptr->fragment.resize(buf.size());
                   std::memcpy(&ptr->fragment[0], buf.get(), buf.size());
                   retval.data->gets.push_back(std::move(ptr));
                   return seastar::stop_iteration::no;
                 })
                 .handle_exception(logerr);
             })
             .handle_exception(logerr);
         })
    .then([retval] {
      // retval keeps a lw_shared_ptr
      return seastar::make_ready_future<wal_read_reply>(retval);
    });
}

}  // namespace smf
