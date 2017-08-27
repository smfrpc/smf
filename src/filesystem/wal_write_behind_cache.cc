#include "filesystem/wal_write_behind_cache.h"

#include <core/metrics.hh>

#include "platform/log.h"

namespace smf {

wal_write_behind_cache::wal_write_behind_cache(
  seastar::file_output_stream_options o)
  : opts(o) {
  namespace sm = seastar::metrics;
  metrics_.add_group(
    "smf::wal_write_behind_cache",
    {sm::make_derive("cache_hits", stats_.hits,
                     sm::description("Number of chache hits")),
     sm::make_derive("bytes_read", stats_.bytes_read,
                     sm::description("Number of bytes writen served")),
     sm::make_derive("evicted_fragments", stats_.evicted_puts,
                     sm::description("Number of evicted tx_put_fragments")),
     sm::make_derive(
       "bytes_written", stats_.bytes_written,
       sm::description("Number of bytes writen to this page cache"))
    });
}

uint64_t wal_write_behind_cache::min_offset() {
  if (puts_.empty()) { return 0; }
  return puts_.begin()->offset;
}
uint64_t wal_write_behind_cache::max_offset() {
  if (puts_.empty()) { return 0; }
  return puts_.end()->offset;
}

void wal_write_behind_cache::put(wal_write_behind_cache::mem_put p) {
  current_size_ += p.fragment->size.size();
  stats_.bytes_written += p.fragment->size();

  // update intrusive indexes
  allocated_.emplace_back(std::move(p));
  puts_.insert(allocated_.back());

  while (current_size_ > opts.preallocation_size) {
    ++stats_.evicted_puts;
    uint64_t virtual_offset = allocated_.front().offset;
    auto     it             = puts_.find(virtual_offset);
    if (it != puts_.end()) { puts_.erase(it); }
    current_size_ -= allocated_.front().data.size();
    allocated_.pop_front();
  }
}

wal_read_reply wal_write_behind_cache::get(const wal_read_request &req) {
  wal_read_reply r;

  auto     offset     = req.req->offset();
  uint32_t bytes_left = req.req->max_bytes();
  while (offset < max_offset() && bytes_left > 0) {
    auto it = puts_.find(offset);
    if (it == puts_.end()) { break; }
    if (bytes_left - it->fragment.size() <= 0) { break; }

    bytes_left -= it->fragment.size();
    offset += it->offset;
    ++stats_.hits;
    stats_.bytes_read += it->fragment.size();
    auto f = std::make_unique<wal::tx_get_fragmentT>();
    // MUST use copy constructor here
    f->hdr = std::make_unique<wal::header>(*(it->fragment->hdr.get()));
    f->fragment.reserve(it->fragment->fragment.size());
    std::copy(it->fragment->fragment.begin(), it->fragment->fragment.end(),
              std::back_inserter(f->fragment));
    r.gets.push_back(std::move(f));
  }
  DLOG_DEBUG_IF(r.empty(), "Request from cache was not found.");
  if (!r.empty()) { r.data->mutate_next_offset(offset); }
  return r;
}

}  // namespace smf
