

namespace smf {
// move to implementation file
// copied from file.hh in seastar
//
constexpr uint64_t KDiskDMAAlignment = 4096;

seastar::future<wal_clock_pro_cache::chunk_t> wal_clock_pro_cache::fetch_page(
  const uint32_t &page, const seastar::io_priority_class &pc) {
  const uint64_t page_offset_begin =
    static_cast<uint64_t>(page) * kDiskDMAAlignment;

  auto bufptr = seastar::allocate_aligned_buffer<char>(kDiskDMAAlignment,
                                                       kDiskDMAAlignment);
  auto fut =
    file_->dma_read(page_offset_begin, bufptr.get(), kDiskDMAAlignment, pc);

  return std::move(fut).then(
    [page, bufptr = std::move(bufptr)](auto size) mutable {
      LOG_THROW_IF(size > kDiskDMAAlignment, "Read more than 1 page");
      chunk_t c(page, page_data(size, std::move(bufptr)));
      return seastar::make_ready_future<chunk_t>(std::move(c));
    });
}


future<wal_page_cache::chunk_t> wal_page_cache::cache_read(
  seastar::lw_shared_ptr<seastar::file> f,
  // this inode is the static_cast ( stat.ino_t )
  uint64_t                          inode,
  uint32_t                          original_page,
  const seastar::io_priority_class &pc) {
  struct __key {
    uint64_t i;
    uint32_t j;
  };
  __key    k{inode, original_page};
  uint64_t page_idx = xxhash_64((char *)&k, sizeof(k));


  if (cache_->contains(page_idx)) {
    return seastar::make_ready_future<chunk_t_ptr>(cache_->get_page(page_idx));
  }
  // we fetch things dynamiclly. so just fill up the buffer before eviction
  // agorithm kicks in. nothing fancy
  //
  // if we just let the clock-pro work w/out pre-allocating, you end up w/ one
  // or 2 pages that you are moving between hot and cold and test
  //
  if (cache_->size() < number_of_pages_) {
    return fetch_page(page_idx, pc).then([this, page_idx](auto chunk) {
      cache_->set(std::move(chunk));
      return seastar::make_ready_future<chunk_t_ptr>(
        cache_->get_chunk_ptr(page_idx));
    });
  }
  cache_->run_cold_hand();
  cache_->run_hot_hand();
  cache_->fix_hands();

  // next add page to list
  return fetch_page(page_idx, pc).then([this, page_idx](auto chunk) {
    cache_->set(std::move(chunk));
    return seastar::make_ready_future<chunk_t_ptr>(
      cache_->get_chunk_ptr(page_idx));
  });
}
}
