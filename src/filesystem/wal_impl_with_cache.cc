#include "filesystem/wal_impl_with_cache.h"
// smf
#include "filesystem/wal_requests.h"

namespace smf {
wal_impl_with_cache::wal_impl_with_cache(wal_opts _opts)
  : wal(std::move(_opts)) {
  writer_ = std::make_unique<wal_writer>(opts.directory, &opts.wstats); // done
  reader_ = std::make_unique<wal_reader>(opts.directory, &opts.rstats); // wip
  cache_ = std::make_unique<wal_mem_cache>(opts.cache_size, &opts.cstats);
}

future<uint64_t> wal_impl_with_cache::append(wal_write_request req) {
  return writer_->append(std::move(req)).then([
    this, bufcpy = req.data.share()
  ](uint64_t epoch) mutable { return cache_->put(epoch, std::move(bufcpy)); });
}

future<> wal_impl_with_cache::invalidate(uint64_t epoch) {
  return cache_->remove(epoch).then(
    [this, epoch] { return writer_->invalidate(epoch); });
}

// this nees to change
future<wal_opts::maybe_buffer> wal_impl_with_cache::get(wal_read_request req) {
  uint64_t offset = req.offset;
  return cache_->get(offset).then([this, req = std::move(req)](auto maybe_buf) {
    if(!maybe_buf) {
      return reader_->get(std::move(req));
    }
    return make_ready_future<wal_opts::maybe_buffer>(std::move(maybe_buf));
  });
}

future<> wal_impl_with_cache::open() {
  return writer_->open().then([this] { return reader_->open(); });
}

future<> wal_impl_with_cache::close() {
  return writer_->close().then([this] { return reader_->close(); });
}

} // namespace smf
