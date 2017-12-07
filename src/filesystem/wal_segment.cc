// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_segment.h"

#include <boost/iterator/counting_iterator.hpp>
#include <core/align.hh>
#include <core/file.hh>
#include <core/reactor.hh>

#include "platform/log.h"

namespace smf {

wal_segment::wal_segment(seastar::sstring                  name,
                         const seastar::io_priority_class &prio,
                         uint32_t                          unflushed_bytes,
                         uint32_t                          concurrent_writes)
  : filename(name)
  , pclass(prio)
  , max_unflushed_bytes(unflushed_bytes)
  , concurrency(concurrent_writes) {
  append_sem_.ensure_space_for_waiters(1);
}

wal_segment::~wal_segment() {
  LOG_ERROR_IF(!closed_, "***POSSIBLE DATA LOSS*** file was not closed. {}",
               filename);
}

seastar::future<>
wal_segment::open() {
  return file_exists(filename).then([this](bool filename_exists) {
    LOG_THROW_IF(filename_exists,
                 "File: {} already exists. Cannot re-open for writes",
                 filename);
    auto flags = seastar::open_flags::rw | seastar::open_flags::create |
                 seastar::open_flags::truncate;
    return seastar::open_file_dma(filename, flags)
      .then([this](seastar::file f) {
        f_ = seastar::make_lw_shared(std::move(f));
        return seastar::make_ready_future<>();
      });
  });
}
seastar::future<>
wal_segment::close() {
  DLOG_THROW_IF(closed_, "File lease already closed. Bug: {}", filename);
  closed_ = true;
  return flush().then([f = f_] { return f->close().finally([f] {}); });
}
seastar::future<>
wal_segment::do_flush() {
  return seastar::do_with(seastar::semaphore(concurrency), [this](auto &limit) {
    return seastar::do_for_each(
             boost::counting_iterator<uint32_t>(0),
             boost::counting_iterator<uint32_t>(chunks_.size()),
             [this, &limit](auto _) mutable {
               return seastar::with_semaphore(limit, 1, [this]() mutable {
                 const auto alignment = f_->disk_write_dma_alignment();
                 auto       x         = chunks_.front();
                 chunks_.pop_front();
                 auto xoffset = seastar::align_down(offset_, alignment);
                 offset_ += x->pos();

                 DLOG_THROW_IF(
                   xoffset > offset_,
                   "Write offset should always align_down: dma_offset: {} , "
                   "current offset_: {} ",
                   xoffset, offset_);

                 if (!x->is_full()) { x->zero_tail(); }
                 DLOG_TRACE("SEGMENT:: dma_offset:{}, "
                            "current_offset:{},previous_offset:{}  size:{}",
                            xoffset, offset_, offset_ - x->pos(), x->size);

                 DLOG_THROW_IF((xoffset & (alignment - 1)) != 0,
                               "Start offset is not page aligned. Severe bug");

                 // send in background - safely!!
                 //
                 f_->dma_write(xoffset, x->data(), x->size, pclass)
                   .then([this, x, xoffset](auto written_bytes) {
                     DLOG_THROW_IF(
                       written_bytes != f_->disk_write_dma_alignment(),
                       "Wrote incorrect number of bytes!: {} . Should "
                       "always "
                       "be page-multiples",
                       written_bytes);
                     if (!x->is_full()) { chunks_.push_back(x); }
                     return seastar::make_ready_future<>();
                   });
               });
             })
      .then([this, &limit] {
        return limit.wait(concurrency).then([this] {
          if ((offset_ & (f_->disk_write_dma_alignment() - 1)) != 0) {
            return f_->truncate(offset_);
          }
          return seastar::make_ready_future<>();
        });
      });
  });
}

seastar::future<>
wal_segment::flush() {
  if (is_fully_flushed_ || chunks_.empty()) {
    return seastar::make_ready_future<>();
  }
  return seastar::with_semaphore(append_sem_, 1,
                                 [this] {
                                   // need to double check
                                   if (chunks_.empty()) {
                                     return seastar::make_ready_future<>();
                                   }
                                   // do the real flush
                                   return do_flush();
                                 })
    .finally([this] {
      bytes_pending_    = 0;
      is_fully_flushed_ = true;
    });
}

seastar::future<>
wal_segment::append(const char *buf, const std::size_t origi_sz) {
  is_fully_flushed_ = false;
  if (chunks_.empty()) {
    chunks_.push_back(
      seastar::make_lw_shared<chunk>(f_->disk_write_dma_alignment()));
  }
  std::size_t n = origi_sz;
  while (n > 0) {
    auto it = *chunks_.rbegin();
    if (it->is_full()) {
      chunks_.push_back(
        seastar::make_lw_shared<chunk>(f_->disk_write_dma_alignment()));
      continue;
    }
    n -= it->append(buf + (origi_sz - n), n);
  }
  bytes_pending_ += origi_sz;

  if (bytes_pending_ < max_unflushed_bytes) {
    return seastar::make_ready_future<>();
  }

  return seastar::with_semaphore(append_sem_, 1, [this] {
    // need to double check
    if (chunks_.empty()) { return seastar::make_ready_future<>(); }
    auto x = chunks_.front();
    chunks_.pop_front();
    bytes_pending_ -= x->size;
    auto xoffset = seastar::align_down(offset_, f_->disk_write_dma_alignment());
    offset_ += x->pos();
    DLOG_THROW_IF(xoffset > offset_,
                  "Write offset should always align_down: dma_offset: {} , "
                  "current offset_: {} ",
                  xoffset, offset_);
    DLOG_THROW_IF((xoffset & (f_->disk_write_dma_alignment() - 1)) != 0,
                  "Start offset is not page aligned. Severe bug");
    f_->dma_write(xoffset, x->data(), x->size, pclass)
      .then([this](auto written_bytes) {
        DLOG_THROW_IF(written_bytes != f_->disk_write_dma_alignment(),
                      "Wrote incorrect number of bytes!: {}", written_bytes);
        return seastar::make_ready_future<>();
      });
    return seastar::make_ready_future<>();
  });
}

}  // namespace smf
