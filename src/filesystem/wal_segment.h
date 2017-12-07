// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <core/aligned_buffer.hh>
#include <core/file.hh>
#include <core/sstring.hh>

#include "platform/log.h"
#include "platform/macros.h"

namespace smf {

/// \brief modeled after seastar/core/fstream.cc:file_data_sink_impl
class wal_segment {
 public:
  class chunk {
   public:
    SMF_DISALLOW_COPY_AND_ASSIGN(chunk);

    explicit chunk(const std::size_t alignment)
      : size(alignment)
      , buf_(seastar::allocate_aligned_buffer<char>(alignment, alignment)) {}

    bool
    is_full() const {
      return pos_ == size;
    }
    std::size_t
    space_left() const {
      return size - pos_;
    }
    std::size_t
    pos() const {
      return pos_;
    }

    std::size_t
    append(const char *src, std::size_t sz) {
      sz = sz > space_left() ? space_left() : sz;
      std::memcpy(buf_.get() + pos_, src, sz);
      pos_ += sz;
      return sz;
    }

    void
    zero_tail() {
      std::memset(buf_.get() + pos_, 0, space_left());
    }

    const char *
    data() const {
      return buf_.get();
    }

    const std::size_t size;

   private:
    std::unique_ptr<char[], seastar::free_deleter> buf_;
    std::size_t                                    pos_{0};
  };

  SMF_DISALLOW_COPY_AND_ASSIGN(wal_segment);
  wal_segment(seastar::sstring                  filename,
              const seastar::io_priority_class &prio,
              uint32_t max_unflushed_bytes          = 1024 * 1024 /*1MB*/,
              uint32_t concurrent_background_writes = 4);
  ~wal_segment();

  seastar::future<> open();
  seastar::future<> close();
  seastar::future<> flush();
  seastar::future<> append(const char *buf, const std::size_t n);

  const seastar::sstring            filename;
  const seastar::io_priority_class &pclass;
  const uint32_t                    max_unflushed_bytes;
  const uint32_t                    concurrency;


 private:
  /// \brief unsafe. use flush()
  seastar::future<> do_flush();

 private:
  bool               closed_{false};
  bool               is_fully_flushed_{true};
  uint64_t           offset_{0};
  uint64_t           bytes_pending_{0};
  seastar::semaphore append_sem_{1};

  std::deque<seastar::lw_shared_ptr<chunk>> chunks_;
  seastar::lw_shared_ptr<seastar::file>     f_;
};

}  // namespace smf
