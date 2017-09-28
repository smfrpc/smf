// Copyright 2017 Alexander Gallego
//

#include "filesystem/wal_partition_manager.h"

#include <algorithm>

#include "filesystem/wal_write_behind_utils.h"
#include "filesystem/wal_write_projection.h"

namespace smf {
wal_partition_manager::wal_partition_manager(wal_opts         o,
                                             seastar::sstring topic_name,
                                             uint32_t         topic_partition)
  : opts(o), topic(topic_name), partition(topic_partition) {
  cache_ = std::make_unique<wal_write_behind_cache>(
    topic, partition, default_file_ostream_options());
  writer_ = std::make_unique<wal_writer>(topic, partition);
  reader_ = std::make_unique<wal_reader>(topic, partition);
}

wal_partition_manager::~wal_partition_manager() {}

wal_partition_manager::wal_partition_manager(wal_partition_manager &&o) noexcept
  : opts(std::move(o.opts))
  , topic(std::move(o.topic))
  , partition(std::move(o.partition))
  , writer_(std::move(o.writer_))
  , reader_(std::move(o.reader_))
  , cache_(std::move(o.cache_)) {}

seastar::future<wal_write_reply> wal_partition_manager::append(
  seastar::lw_shared_ptr<wal_write_projection> p) {
  DLOG_THROW_IF(!is_ready_open_, "wal_partition_manager::append... not ready!");
  return writer_->append(p).then([this, p](auto reply) {
    auto idx = reply.data.start_offset;
    std::for_each(p->projection.begin(), p->projection.end(),
                  [&idx, this](auto it) {
                    cache_->put(idx, it);
                    idx += it->size();
                  });
    return seastar::make_ready_future<wal_write_reply>(std::move(reply));
  });
}


seastar::future<wal_read_reply> wal_partition_manager::get(wal_read_request r) {
  DLOG_THROW_IF(!is_ready_open_, "wal_partition_manager::get... not ready!");
  if (r.req->offset() >= cache_->min_offset()
      && r.req->offset() <= cache_->max_offset()) {
    return seastar::make_ready_future<wal_read_reply>(cache_->get(r));
  }
  return reader_->get(r);
}
seastar::future<> wal_partition_manager::do_open() {
  return reader_->open()
    .handle_exception([this](auto eptr) {
      LOG_ERROR("Error opening reader: {}. for {}.{} ", eptr, topic, partition);
      std::rethrow_exception(eptr);
    })
    .then([this] {
      return writer_->open().handle_exception([this](auto eptr) {
        LOG_ERROR("Error opening writer: {}. for {}.{} ", eptr, topic,
                  partition);
        std::rethrow_exception(eptr);
      });
    })
    .finally([this] { is_ready_open_ = true; });
}
seastar::future<> wal_partition_manager::open() {
  LOG_DEBUG("Opening partition manager with: opts={}, topic={}, partition={}",
            opts, topic, partition);
  seastar::sstring dir = topic + "." + seastar::to_sstring(partition);
  return file_exists(dir)
    .then([dir](bool exists) {
      if (exists) { return seastar::make_ready_future<>(); }
      return seastar::make_directory(dir).then_wrapped(
        [](auto _) { return seastar::make_ready_future<>(); });
    })
    .then_wrapped([this](auto _) { return this->do_open(); });
}
seastar::future<> wal_partition_manager::close() {
  LOG_DEBUG("Closing partition manager with: opts={}, topic={}, partition={}",
            opts, topic, partition);
  auto elogger = [](auto eptr) {
    LOG_ERROR("Ignoring error closing partition manager: {}", eptr);
    return seastar::make_ready_future<>();
  };
  return writer_->close().handle_exception(elogger).then(
    [this, elogger] { return reader_->close().handle_exception(elogger); });
}


}  // namespace smf
