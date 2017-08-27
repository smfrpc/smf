#include "filesystem/wal_parition_manager.h"

#include "filesystem/wal_write_behind_utils.h"
#include "filesystem/wal_write_projection.h"

namespace smf {
wal_partition_manager::wal_partition_manager(wal_opts o,
                                             sstring  topic_name,
                                             uint32_t topic_partition)
  : opts(o), topic(topic_name), partition(topic_partition) {
  cache_ =
    std::make_unique<wal_write_behind_cache>(default_file_ostream_options());
  writer_ = std::make_unique<wal_writer>(topic_name, topic_partition);
  reader_ = std::make_unique<wal_writer>(topic_name, topic_partition);
}

wal_partition_manager::~wal_partition_manager() {}

wal_partition_manager::wal_partition_manager(wal_partition_manager &&o) noexcept
  : opts(std::move(o.opts))
  , topic(std::move(o.topic))
  , partition(std::move(o.parition))
  , writer_(std::move(o.writer_))
  , reader_(std::move(o.reader_))
  , cache_(std::move(o.cache_)) {}

seastar::future<wal_write_reply> wal_partition_manager::append(
  wal_write_request r) {
  // 1) Get projection
  // 2) Write to disk
  // 3) Write to to cache
  // 4) Return reduced state
  return seastar::map_reduce(r.begin(), r.end(), [this](auto it) {
    // get the iterator, and get the address of the deref type
    auto p = wal_write_projection::translate(&(*it));
    return writer_->append(p).then(
      [this, p](auto reply) {
        auto idx = reply.start_offset;
        std::foreach (p->projection.begin(), p->projection.end(),
                      [&idx, this](auto it) {
                        idx += it->size();
                        cache_->put({idx, it->fragment});
                      });
        return seastar::make_ready_future<decltype(reply)>(std::move(reply));
      },
      wal_write_reply(0, 0),
      [this](auto acc, auto next) {
        if (acc.data->start_offset == 0) {
          acc.data->start_offset = next.data->start_offset;
        }
        acc.data->end_offset = next.data->end_offset;
        return acc;
      });
}


seastar::future<wal_read_reply> get(wal_read_request r) {
    if (r.req->offset() >= cache_->min_offset()
        && r.req->offset() <= cache_->max_offset()) {
      return seastar::make_ready_future<wal_write_reply>(cache_->get(r));
    }
    return reader_->get(r);
}
seastar::future<> wal_partition_manager::open() {
    LOG_DEBUG("Opening parition manager with: opts={}, topic={}, parition={}",
              opts, topic, partition);
    return writer_->open().then([this] { return reader_->open(); });
}
seastar::future<> wal_partition_manager::close() {
    LOG_DEBUG("Closing parition manager with: opts={}, topic={}, parition={}",
              opts, topic, partition);
    auto elogger = [](auto eptr) {
      LOG_ERROR("Ignoring error closing partition manager: {}", eptr);
      return seastar::make_ready_future<>();
    };
    return writer_->close().handle_exception(elogger).then(
      [this] { return reader_->close().handle_exception(elogger); });
}


}  // namespace smf
