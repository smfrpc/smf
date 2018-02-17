// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/write_ahead_log.h"

#include <memory>
#include <utility>

#include <sys/sdt.h>

#include "platform/log.h"

#include "filesystem/wal_requests.h"
#include "filesystem/wal_write_projection.h"
#include "utils/checks/disk.h"

namespace smf {
write_ahead_log::write_ahead_log(wal_opts _opts)
  : opts(std::move(_opts)), tm_(opts) {}

seastar::future<seastar::lw_shared_ptr<wal_write_reply>>
write_ahead_log::append(wal_write_request r) {
  DLOG_THROW_IF(r.runner_core != seastar::engine().cpu_id(),
                "Incorrect core assignment");
  DLOG_THROW_IF(!wal_write_request::valid(r), "invalid write request");
  DTRACE_PROBE(smf, wal_write);
  // 1) Get projection
  // 2) Write to disk
  // 3) Write to to cache
  // 4) Return reduced state

  return seastar::map_reduce(
    // for-all
    r.begin(), r.end(),

    // Mapper function
    [this, r](const smf::wal::tx_put_partition_tuple *it) {
      auto topic = seastar::sstring(r.req->topic()->c_str());
      return this->tm_.get_manager(topic, it->partition())
        .then([this, it](auto mngr) { return mngr->append(it); });
    },

    // Initial State
    seastar::make_lw_shared<wal_write_reply>(),

    // Reducer function
    [this](auto acc, auto next) {
      for (const auto &t : *next) {
        auto &p = t.second;
        acc->set_reply_partition_tuple(p->topic, p->partition, p->start_offset,
                                       p->end_offset);
      }
      return acc;
    });
}

seastar::future<seastar::lw_shared_ptr<wal_read_reply>>
write_ahead_log::get(wal_read_request r) {
  DLOG_THROW_IF(!wal_read_request::valid(r), "invalid read request");
  DTRACE_PROBE(smf, wal_get);
  auto t = seastar::sstring(r.req->topic()->c_str());
  return tm_.get_manager(t, r.req->partition()).then([r](auto mngr) {
    return mngr->get(r);
  });
}

seastar::future<>
write_ahead_log::open() {
  LOG_INFO("starting: {}", opts);
  if (seastar::engine().cpu_id() == 0) {
    auto dir = opts.directory;
    return file_exists(dir).then([dir](bool exists) {
      if (exists) { return seastar::make_ready_future<>(); }
      LOG_INFO("Creating log directory: {}", dir);
      return seastar::make_directory(dir).then([dir] {
        LOG_INFO("Checking for supported filesystems");
        return smf::checks::disk::check(dir);
      });
    });
  }
  return seastar::make_ready_future<>();
}

seastar::future<>
write_ahead_log::close() {
  LOG_INFO("stopping: {}", opts);
  // close all topic managers
  return tm_.close();
}

}  // namespace smf
