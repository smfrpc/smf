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
  struct page_data {
    using bufptr_t = std::unique_ptr<char[], free_deleter>;
    page_data(uint32_t size, bufptr_t d) : buf_size(size), data(std::move(d)) {}
    page_data(page_data &&d) noexcept : buf_size(std::move(d.buf_size)),
                                        data(std::move(d.data)) {}
    const uint32_t buf_size;
    bufptr_t       data;
    SMF_DISALLOW_COPY_AND_ASSIGN(page_data);
  };
  using cache_t     = smf::clock_pro_cache<uint64_t, page_data>;
  using chunk_t     = typename cache_t::chunk_t;
  using chunk_t_ptr = typename std::add_pointer<chunk_t>::type;


  /// \brief - designed to be a cache from the direct-io layer
  /// and pages fetched. It understands the wal_writer format
  wal_clock_pro_cache(lw_shared_ptr<file> f,
                      // size of the `f` file from the fstat() call
                      // need this to figure out how many pages to allocate
                      int64_t size,
                      // max_pages_in_memory = 0, allows us to make a decision
                      // impl defined. right now, it chooses the max of 10% of
                      // the file or 10 pages. each page is 4096 bytes
                      uint32_t max_pages_in_memory = 0);

  const int64_t  file_size;
  const uint32_t number_of_pages;

  /// \brief - return buffer for offset with size
  future<wal_read_reply> read(wal_read_request r);

  /// \brief - closes lw_share_ptr<file>
  future<> close();

 private:
  /// \brief - returns a temporary buffer of size. similar to
  /// isotream.read_exactly()
  /// different than a wal_read_request for arguments since it returns **one**
  /// temp buffer
  ///
  future<temporary_buffer<char>> read_exactly(int64_t                  offset,
                                              int64_t                  size,
                                              const io_priority_class &pc);
  /// \brief actual reader func
  /// recursive
  future<lw_shared_ptr<wal_read_reply>> do_read(wal_read_request r);

  /// \breif fetch exactly one page from disk w/ dma_alignment() so that
  /// we don't pay the penalty of fetching 2 pages and discarding one
  future<chunk_t> fetch_page(const uint32_t &page, const io_priority_class &pc);

  /// \brief perform the clock-pro caching and eviction techniques
  /// and then return a ptr to the page
  /// intrusive containers::iterators are not default no-throw move
  /// constructible, so we have to get the ptr to the data which is lame
  ///
  future<chunk_t_ptr> clock_pro_get_page(uint32_t                 page,
                                         const io_priority_class &pc);

 private:
  lw_shared_ptr<file>      file_;  // uses direct io for fetching
  uint32_t                 max_resident_pages_;
  std::unique_ptr<cache_t> cache_;
};


}  // namespace smf
