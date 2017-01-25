// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <cmath>
#include <list>
#include <utility>
// third party
#include <boost/dynamic_bitset.hpp>
#include <boost/intrusive/options.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <core/file.hh>

namespace smf {
/// TODO: add a semaphore for memory_limits of this cache
/// so that you don't blow up all of memory.
///
class lazy_file {
 public:
  struct lazy_file_chunk : public boost::intrusive::set_base_hook<>,
                           public boost::intrusive::unordered_set_base_hook<> {
    lazy_file_chunk(uint32_t _page_no, temporary_buffer<char> _data)
      : page_no(_page_no), data(std::move(_data)) {}
    const uint32_t         page_no;
    temporary_buffer<char> data;
  };

  struct lazy_file_chunk_intrusive_key {
    typedef uint32_t type;
    const type &operator()(const lazy_file_chunk &v) const { return v.page_no; }
  };
  // Define omap like ordered and unordered classes
  using intrusive_chunk_key =
    boost::intrusive::key_of_value<lazy_file_chunk_intrusive_key>;
  using intrusive_chunk_map =
    boost::intrusive::set<lazy_file_chunk, intrusive_chunk_key>;

  static_assert(std::is_same<intrusive_chunk_map::key_type, uint32_t>::value,
                "bad key for intrusive map");

 public:
  lazy_file(lw_shared_ptr<file> f, size_t size)
    : file_size(size)
    , file_(std::move(f))
    , pages_(std::ceil(file_size / file_->disk_read_dma_alignment())) {}

  const size_t          file_size;
  inline const uint32_t number_of_pages() const { return pages_.size(); }

  /// \brief - return buffer for offset with size
  future<temporary_buffer<char>> read(
    uint64_t                 offset,
    uint64_t                 size,
    const io_priority_class &pc = default_priority_class());

  future<> close() {
    // must hold file until handle closes
    return file_->close().then([this] { return make_ready_future<>(); });
  }

 private:
  future<temporary_buffer<char>> read_from_cache(uint64_t offset,
                                                 uint64_t size);
  future<> fetch_pages(uint64_t                 offset,
                       uint64_t                 number_of_pages,
                       const io_priority_class &pc);

 private:
  lw_shared_ptr<file> file_;
  // used for holding the memory references
  std::list<lazy_file_chunk> allocated_pages_;
  // used for indexing
  intrusive_chunk_map sorted_chunks_;
  // used for set belonging testing
  boost::dynamic_bitset<> pages_;
};


}  // namespace smf
