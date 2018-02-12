// Copyright 2017 Alexander Gallego
//

#include "filesystem/wal_partition_manager.h"

#include <algorithm>

#include <core/reactor.hh>

#include "filesystem/wal_write_behind_utils.h"
#include "filesystem/wal_write_projection.h"

namespace smf {
wal_partition_manager::wal_partition_manager(wal_opts         o,
                                             seastar::sstring topic_name,
                                             uint32_t         topic_partition)
  : opts(std::move(o))
  , topic(topic_name)
  , partition(topic_partition)
  , work_dir(opts.directory + "/" + topic + "." +
             seastar::to_sstring(partition))
  , writer_(opts, work_dir)
  , reader_(work_dir)
  , cache_(topic, partition, default_file_ostream_options()) {}

wal_partition_manager::~wal_partition_manager() {}

wal_partition_manager::wal_partition_manager(wal_partition_manager &&o) noexcept
  : opts(std::move(o.opts))
  , topic(std::move(o.topic))
  , partition(std::move(o.partition))
  , writer_(std::move(o.writer_))
  , reader_(std::move(o.reader_))
  , cache_(std::move(o.cache_)) {}

seastar::future<seastar::lw_shared_ptr<wal_write_reply>>
wal_partition_manager::append(seastar::lw_shared_ptr<wal_write_projection> p) {
  return writer_.append(p).then([this, p](auto reply) {
    DLOG_THROW_IF(reply->size() != 1,
                  "The cache for *this* topic: `{}` partition: `{}` tuple "
                  "should be exactly one",
                  topic, partition);
    auto idx = reply->begin()->second->start_offset;
    for (auto &&i : p->projection) {
      auto sz = i->on_disk_size();
      cache_.put(idx, std::move(i));
      idx += sz;
    }
    return seastar::make_ready_future<decltype(reply)>(reply);
  });
}


seastar::future<seastar::lw_shared_ptr<wal_read_reply>>
wal_partition_manager::get(wal_read_request r) {
  if (cache_.is_offset_in_range(r.req->offset())) {
    return seastar::make_ready_future<seastar::lw_shared_ptr<wal_read_reply>>(
      cache_.get(r));
  }
  return reader_.get(r);
}
seastar::future<>
wal_partition_manager::do_open() {
  return reader_.open()
    .handle_exception([this](auto eptr) {
      LOG_ERROR("Error opening reader: {}. for {}.{} ", eptr, topic, partition);
      std::rethrow_exception(eptr);
    })
    .then([this] {
      return writer_.open().handle_exception([this](auto eptr) {
        LOG_ERROR("Error opening writer: {}. for {}.{} ", eptr, topic,
                  partition);
        std::rethrow_exception(eptr);
      });
    });
}
seastar::future<>
wal_partition_manager::open() {
  LOG_DEBUG("Opening partition manager with: opts={}, topic={}, partition={}, "
            "work_dir={}",
            opts, topic, partition, work_dir);
  return seastar::file_exists(work_dir)
    .then([dir = work_dir](bool exists) {
      if (exists) { return seastar::make_ready_future<>(); }
      return seastar::make_directory(dir).then(
        [] { return seastar::make_ready_future<>(); });
    })
    .then([this] { return this->do_open(); });
}
seastar::future<>
wal_partition_manager::close() {
  LOG_DEBUG("Closing partition manager with: opts={}, topic={}, partition={}",
            opts, topic, partition);
  auto elogger = [](auto eptr) {
    LOG_ERROR("Ignoring error closing partition manager: {}", eptr);
    return seastar::make_ready_future<>();
  };
  return writer_.close().handle_exception(elogger).then(
    [this, elogger] { return reader_.close().handle_exception(elogger); });
}


}  // namespace smf
