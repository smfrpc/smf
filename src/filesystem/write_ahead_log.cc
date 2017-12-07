// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/write_ahead_log.h"

#include <memory>
#include <utility>

#include "platform/log.h"

// #include "filesystem/wal_file_name_mender.h"
// TODO(need a partition scanner, etc)
#include "filesystem/wal_requests.h"
#include "filesystem/wal_write_projection.h"

namespace smf {
write_ahead_log::write_ahead_log(wal_opts _opts)
  : opts(std::move(_opts)), tm_(opts) {}

seastar::future<wal_write_reply> write_ahead_log::append(wal_write_request r) {
  // 1) Get projection
  // 2) Write to disk
  // 3) Write to to cache
  // 4) Return reduced state

  return seastar::map_reduce(
    // for-all
    r.begin(), r.end(),

    // Mapper function
    [this, r](auto it) {
      auto topic = seastar::sstring(r.req->topic()->c_str());
      return this->tm_.get_manager(topic, it->partition()).then([
        this, topic, projection = wal_write_projection::translate(*it)
      ](auto mngr) { return mngr->append(projection); });
    },

    // Initial State
    wal_write_reply(0, 0),

    // Reducer function
    [this](auto acc, auto next) {
      acc.data.start_offset =
        std::min(acc.data.start_offset, next.data.start_offset);
      acc.data.end_offset = std::max(acc.data.end_offset, next.data.end_offset);
      return acc;
    });
}

seastar::future<wal_read_reply> write_ahead_log::get(wal_read_request r) {
  auto t = seastar::sstring(r.req->topic()->c_str());
  return tm_.get_manager(t, r.req->partition()).then([r](auto mngr) {
    return mngr->get(r);
  });
}

seastar::future<> write_ahead_log::open() {
  LOG_INFO("starting: {}", opts);
  return seastar::make_ready_future<>();
  auto dir = opts.directory;
  // TODO(agallego)
  // missing partition scanning
  return file_exists(dir).then([dir](bool exists) {
    if (exists) { return seastar::make_ready_future<>(); }
    return seastar::make_directory(dir).then_wrapped(
      [](auto _) { return seastar::make_ready_future<>(); });
  });
}

seastar::future<> write_ahead_log::close() {
  LOG_INFO("stopping: {}", opts);
  // close all topic managers
  return tm_.close();
}

}  // namespace smf
