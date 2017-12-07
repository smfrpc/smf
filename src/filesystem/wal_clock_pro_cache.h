// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <cmath>
#include <list>
#include <utility>
// third party
#include <core/aligned_buffer.hh>
#include <core/file.hh>
// smf
#include "filesystem/wal_requests.h"
#include "platform/macros.h"
#include "utils/caching/clock_pro/clock_pro.h"

namespace smf {
class wal_clock_pro_cache {
 public:


  /// \brief this is risky and is for performance reasons
  /// we do not want to fs::stat files all the time.
  /// This recomputes offsets of files that we want to read
  void update_file_size_by(uint64_t delta);
  int64_t  file_size() { return file_size_; }
  uint32_t number_of_pages() { return number_of_pages_; }

  /// \brief - return buffer for offset with size
  seastar::future<wal_read_reply> read(wal_read_request r,
                                       uint64_t         relative_offset);

  /// \brief - closes lw_share_ptr<file>
  seastar::future<> close();

 private:

  /// \brief - returns a temporary buffer of size. similar to
  /// isotream.read_exactly()
  /// different than a wal_read_request for arguments since it returns **one**
  /// temp buffer
  ///
  seastar::future<seastar::temporary_buffer<char>> read_exactly(
    int64_t offset, int64_t size, const seastar::io_priority_class &pc);

  /// \breif fetch exactly one page from disk w/ dma_alignment() so that
  /// we don't pay the penalty of fetching 2 pages and discarding one
  seastar::future<chunk_t> fetch_page(const uint32_t &                  page,
                                      const seastar::io_priority_class &pc);

  /// \brief perform the clock-pro caching and eviction techniques
  /// and then return a ptr to the page
  /// intrusive containers::iterators are not default no-throw move
  /// constructible, so we have to get the ptr to the data which is lame
  ///
  seastar::future<chunk_t_ptr> clock_pro_get_page(
    uint32_t page, const seastar::io_priority_class &pc);

 private:
  seastar::lw_shared_ptr<seastar::file> file_;  // uses direct io for fetching
};


}  // namespace smf
