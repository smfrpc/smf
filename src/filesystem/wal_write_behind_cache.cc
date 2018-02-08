// Copyright 2017 Alexander Gallego
//

#include "filesystem/wal_write_behind_cache.h"

#include <core/metrics.hh>
#include <core/sstring.hh>

#include "flatbuffers/wal_generated.h"
#include "platform/log.h"

namespace smf {

wal_write_behind_cache::wal_write_behind_cache(
  seastar::sstring                    topic_name,
  uint32_t                            topic_partition,
  seastar::file_output_stream_options o)
  : opts(o) {
  namespace sm = seastar::metrics;
  metrics_.add_group(
    "smf::wal_write_behind_cache::" + topic_name + "(" +
      seastar::to_sstring(topic_partition) + ")",
    {sm::make_derive("cache_hits", stats_.hits,
                     sm::description("Number of chache hits")),
     sm::make_derive("bytes_read", stats_.bytes_read,
                     sm::description("Number of bytes writen served")),
     sm::make_derive("evicted_fragments", stats_.evicted_puts,
                     sm::description("Number of evicted tx_put_fragments")),
     sm::make_derive(
       "bytes_written", stats_.bytes_written,
       sm::description("Number of bytes writen to this page cache"))});
}

uint64_t
wal_write_behind_cache::min_offset() {
  if (keys_.empty()) { return 0; }
  return keys_[0];
}
uint64_t
wal_write_behind_cache::max_offset() {
  if (keys_.empty()) { return 0; }
  return keys_[keys_.size() - 1];
}
void
wal_write_behind_cache::put(uint64_t offset, item_ptr data) {
  while (current_size_ + data->on_disk_size() > opts.preallocation_size &&
         !puts_.empty()) {
    DLOG_INFO("Deleting data because preallocated cache: {} and current_size: "
              "{} and req size: {}",
              opts.preallocation_size, current_size_, data->on_disk_size());

    DLOG_THROW_IF(
      keys_.empty(),
      "flat_hash_map of values and std::set of keys are inconsistent");
    uint64_t k        = keys_[0];
    auto     map_iter = puts_.find(k);
    DLOG_THROW_IF(map_iter == puts_.end(), "Could not find offset: {} in cache",
                  k);
    // main
    current_size_ -= map_iter->second->on_disk_size();

    // accoutning
    puts_.erase(map_iter);
    std::swap(keys_[0], keys_[keys_.size() - 1]);
    keys_.pop_back();
  }
  current_size_ += data->on_disk_size();
  stats_.bytes_written += data->on_disk_size();
  puts_.emplace(offset, data);
  keys_.push_back(offset);
}

seastar::lw_shared_ptr<wal_read_reply>
wal_write_behind_cache::get(const wal_read_request &req) {
  DLOG_THROW_IF(
    !is_offset_in_range(req.req->offset()),
    "Requesting offset not in range of cache. Request offset: {}, cache "
    "min_offset: {}, cache max_offset: {}, request max_bytes: {}",
    req.req->offset(), min_offset(), max_offset(), req.req->max_bytes());

  auto retval = seastar::make_lw_shared<wal_read_reply>(req.req->offset());

  for (auto it = puts_.find(retval->get_consume_offset()); it != puts_.end();
       it      = puts_.find(retval->get_consume_offset())) {
    DLOG_INFO("Getting data: read_offset:{}, size on disk: {}",
              retval->get_consume_offset(), retval->on_wire_size());

    if (retval->on_wire_size() >= req.req->max_bytes()) { break; }
    ++stats_.hits;
    stats_.bytes_read += it->second->on_disk_size();
    auto f = std::make_unique<wal::tx_get_fragmentT>();
    f->hdr = std::make_unique<wal::wal_header>(it->second->hdr);
    f->fragment.resize(it->second->fragment.size());
    std::copy_n(it->second->fragment.get(), it->second->fragment.size(),
                f->fragment.begin());
    retval->update_consume_offset(retval->get_consume_offset() +
                                  it->second->fragment.size() +
                                  kOnDiskInternalHeaderSize);
    retval->reply()->gets.push_back(std::move(f));
  }
  DLOG_ERROR_IF(retval->empty(),
                "Request to cache was not found. Request offset: {}, cache "
                "min_offset: {}, cache max_offset: {}, request max_bytes: {}",
                req.req->offset(), min_offset(), max_offset(),
                req.req->max_bytes());
  return retval;
}

}  // namespace smf
