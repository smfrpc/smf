#include "filesystem/wal_parition_manager.h"

namespace smf {
wal_partition_manager::wal_partition_manager(wal_opts otps,
                                             sstring  topic_name,
                                             uint32_t topic_partition)
  : topic(topic_name), partition(topic_partition) {}

wal_partition_manager::~wal_partition_manager() {}

wal_partition_manager::wal_partition_manager(wal_partition_manager &&o) noexcept
  : topic(std::move(o.topic))
  , partition(std::move(o.parition))
  , writer_(std::move(o.writer_))
  , reader_(std::move(o.reader_))
  , cache_(std::move(o.cache_)) {}

seastar::future<wal_write_reply> wal_partition_manager::append(
  wal_write_request             r) {}
seastar::future<wal_read_reply> get(wal_read_request r) {}
seastar::future<> wal_partition_manager::open() {
  LOG_DEBUG("Opening parition manager with: opts={}, topic={}, parition={}",
            opts, topic, partition);
  return writer_->open().then(
    [this] { return reader_->open().then([this] { return cache_->open(); }); });
}
seastar::future<> wal_partition_manager::close() {
  LOG_DEBUG("Closing parition manager with: opts={}, topic={}, parition={}",
            opts, topic, partition);
  auto elogger = [](auto eptr) {
    LOG_ERROR("Ignoring error closing partition manager: {}", eptr);
    return seastar::make_ready_future<>();
  };
  return writer_->close().handle_exception(elogger).then([this] {
    return reader_->close().handle_exception(elogger).then(
      [this] { return cache_->close().handle_exception(elogger); });
  });
}


}  // namespace smf
