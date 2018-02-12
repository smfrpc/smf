// Copyright 2017 Alexander Gallego
//

#include "filesystem/wal_write_behind_cache.h"

#include <sys/sdt.h>

#include <core/metrics.hh>
#include <core/sstring.hh>

#include "flatbuffers/wal_generated.h"
#include "platform/log.h"

namespace smf {

struct get_lower_functor {
  bool
  operator()(const uint64_t &                            offset,
             const wal_write_behind_cache::eviction_key &kk) {
    return offset < kk.key;
  }
  bool
  operator()(const wal_write_behind_cache::eviction_key &kk,
             const uint64_t &                            offset) {
    return kk.key < offset;
  }
  bool
  operator()(const wal_write_behind_cache::eviction_key &x,
             const wal_write_behind_cache::eviction_key &y) {
    return x.key < y.key;
  }
};

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


wal_write_behind_cache::wal_write_behind_cache(
  wal_write_behind_cache &&o) noexcept
  : stats_(std::move(o.stats_))
  , current_size_(o.current_size_)
  , data_(std::move(o.data_))
  , metrics_(std::move(o.metrics_)){};

uint64_t
wal_write_behind_cache::min_offset() {
  if (data_.empty()) { return 0; }
  return data_.front().key;
}
uint64_t
wal_write_behind_cache::max_offset() {
  if (data_.empty()) { return 0; }
  return data_.back().key;
}
void
wal_write_behind_cache::put(uint64_t offset, item_ptr data) {
  auto const size_on_disk = data->on_disk_size();
  auto       predicate    = [this, size_on_disk]() {
    return current_size_ + size_on_disk > opts.preallocation_size;
  };
  if (predicate()) {
    // compute how many things we are going to evict
    // take into account the ammount of data to save
    if (SMF_LIKELY(!data_.empty())) {
      while (predicate() && !data_.empty()) {
        DTRACE_PROBE(smf, wal_write_behind_cache_evicting_put);
        current_size_ -= data_.front().size;
        data_.pop_front();
      }
    }
  }
  current_size_ += size_on_disk;
  stats_.bytes_written += size_on_disk;
  data_.push_back({offset, size_on_disk, std::move(data)});
}

seastar::lw_shared_ptr<wal_read_reply>
wal_write_behind_cache::get(const wal_read_request &req) {
  DLOG_THROW_IF(
    !is_offset_in_range(req.req->offset()),
    "Requesting offset not in range of cache. Request offset: {}, cache "
    "min_offset: {}, cache max_offset: {}, request max_bytes: {}",
    req.req->offset(), min_offset(), max_offset(), req.req->max_bytes());

  auto retval = seastar::make_lw_shared<wal_read_reply>(req.req->offset());

  for (auto it =
         std::lower_bound(data_.begin(), data_.end(),
                          retval->get_consume_offset(), get_lower_functor{});
       it != std::end(data_);
       it =
         std::lower_bound(data_.begin(), data_.end(),
                          retval->get_consume_offset(), get_lower_functor{})) {
    if (it->key != retval->get_consume_offset()) { break; }
    if (retval->on_wire_size() >= req.req->max_bytes()) { break; }

    DLOG_INFO("Getting data: read_offset:{}, size on disk: {}",
              retval->get_consume_offset(), retval->on_wire_size());

    ++stats_.hits;
    stats_.bytes_read += it->data->on_disk_size();
    auto f = std::make_unique<wal::tx_get_fragmentT>();
    f->hdr = std::make_unique<wal::wal_header>(it->data->hdr);
    f->fragment.resize(it->data->fragment.size());
    std::copy_n(it->data->fragment.get(), it->data->fragment.size(),
                f->fragment.begin());
    retval->update_consume_offset(retval->get_consume_offset() +
                                  it->data->fragment.size() +
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
