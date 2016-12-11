#pragma once
// third party
#include <core/file.hh>
#include <boost/dynamic_bitset.hpp>

namespace smf {

struct file_chunk : intrusive::hook<> {
  file_chunk(uint32_t _page_no, temporary_buffer<char> _data)
    : page_no(_page_no), data(std::move(_data)) {}
  const uint32_t page_no;
  temporary_buffer<char> data;
};

/// TODO(agallego) add a semaphore for memory limits of this cache
/// so that you don't blow up all of memory.
///
class lazy_file {
  public:
  lazy_file(file f, size_t size)
    : file_(std::move(f))
    , file_size(size)
    , pages(std::ceil(file_size / file_.disk_read_dma_alignment())) {}

  const size_t file_size;
  inline const uint32_t number_of_pages() const { return pages_.size(); }
  future<temporary_buffer<char>> read(uint64_t offset, uint64_t size);

  private:
  inline const uint32_t ofset_to_page(uint64_t offset) const {
    assert(offset < file_size);
    auto ret = std::floor(offset / number_of_pages());
  }

  private:
  // used for holding the memory references
  std::vector<file_chunk> allocated_pages_;
  // used for indexing
  boost::intrusive_list<file_chunk> sorted_files_;
  // used for set belonging testing
  boost::dynamic_bitset<> pages_;
};


lazy_file::future<temporary_buffer<char>> read(uint64_t offset, uint64_t size) {
  // test if in pages_
  auto page = offset / pages.size();


  if(pages[page] & 1) {
    // return read with data
    return make_ready_future<>();
  } else {
    // open the page & cache it
    // insert it into the intrusive list
    // sort the intrusive list
    auto aligned_begin = offset & (file.disk_read_dma_alignment() - 1);
    auto aligned_end = file.disk_read_dma_alignment();
    auto buf = file::dma_read_bulk(aligned_begin, aligned_end);
    pages_[aligned_begin / pages_.size()] = 1;
    // etc
    std::sort(sorted_files_);
    return read(offset, size);
  }
}

} // namespace smf
