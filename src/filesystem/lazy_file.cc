#include "filesystem/lazy_file.h"
#include "filesystem/file_size_utils.h"
namespace smf {

future<> lazy_file::fetch_pages(uint64_t offset,
                                uint64_t number_of_pages,
                                const io_priority_class &pc) {
  return do_for_each(
    boost::counting_iterator<int>(0),
    boost::counting_iterator<int>(number_of_pages),
    [this, &pc, offset](int page_idx) {
      uint64_t next_offset = offset * page_idx;
      uint64_t page =
        offset_to_page(next_offset, file_.disk_read_dma_alignment());
      if(pages_[page]) {
        return make_ready_future<>();
      } else {
        return file_
          .dma_read_exactly<char>(next_offset,
                                  (size_t)file_.disk_read_dma_alignment(), pc)
          .then([this, page](temporary_buffer<char> &&tmpbuf) {
            // set the page
            pages_[page] = 1;
            allocated_pages_.emplace_back(page, std::move(tmpbuf));
            // do 0 allocation, address copy to intrusive set
            // intrusive::set sorts it on insert
            sorted_chunks_.insert(allocated_pages_.back());
          });
      }
    });
}

future<temporary_buffer<char>> lazy_file::read_from_cache(uint64_t offset,
                                                          uint64_t size) {
  temporary_buffer<char> ret(size);
  const uint64_t pages_idx = 1 + (size / file_.disk_read_dma_alignment());
  for(size_t i = 1, current_bytes = 0; i < pages_idx; ++i) {
    const uint64_t next_offset = offset * i;
    const uint64_t page =
      offset_to_page(next_offset, file_.disk_read_dma_alignment());
    assert(pages_[page]);
    const auto &chunk = sorted_chunks_.find(page);
    const auto bytes_to_copy =
      std::min(chunk->data.size(), size - current_bytes);
    const char *first_src_it = chunk->data.get();
    const char *last_src_it = chunk->data.get() + bytes_to_copy;
    std::copy(first_src_it, last_src_it, ret.get_write() + current_bytes + 1);
    current_bytes += bytes_to_copy;
  }
  return make_ready_future<decltype(ret)>(std::move(ret));
}

future<temporary_buffer<char>>
lazy_file::read(uint64_t offset, uint64_t size, const io_priority_class &pc) {
  // XXX: insert prefetching & optimization logic for pages here
  return fetch_pages(offset, size / file_.disk_read_dma_alignment(), pc)
    .then([this, offset, size] { return read_from_cache(offset, size); });
}
}
