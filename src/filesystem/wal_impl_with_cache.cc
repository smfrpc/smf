// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_impl_with_cache.h"
// std
#include <memory>
#include <utility>

#include <core/reactor.hh>

// #include "filesystem/wal_file_name_mender.h" TODO(need a parition scanner,
// etc)
#include "filesystem/wal_requests.h"

namespace smf {
wal_impl_with_cache::wal_impl_with_cache(wal_opts _opts)
  : opts(std::move(_opts)) {}

seastar::future<wal_write_reply> wal_impl_with_cache::append(
  wal_write_request req) {
  // 1) Get projection
  // 2) Write to disk
  // 3) Write to to cache
  // 4) Return reduced state
  return seastar::map_reduce(r.begin(), r.end(), [this](auto it) {
    // get the iterator, and get the address of the deref type
    auto p = wal_write_projection::translate(&(*it));
    return writer_->append(p).then(
      [this, p](auto reply) {
        auto idx = reply.data->start_offset;
        std::foreach (p->projection.begin(), p->projection.end(),
                      [&idx, this](auto it) {
                        cache_->put({idx, it->fragment});
                        idx += it->size();
                      });
        return seastar::make_ready_future<decltype(reply)>(std::move(reply));
      },
      wal_write_reply(0, 0),
      [this](auto acc, auto next) {
        acc.data->start_offset =
          std::min(acc.data->start_offset, next.data->start_offset);
        acc.data->end_offset =
          std::max(acc.data->end_offset, next.data->end_offset);
        return acc;
      });

}

seastar::future<wal_read_reply::maybe> wal_impl_with_cache::get(
  wal_read_request req) {
    uint64_t offset = req.offset;
    return cache_->get(offset).then(
      [this, req = std::move(req)](auto maybe_buf) {

        if (!maybe_buf) { return reader_->get(std::move(req)); }

        return seastar::make_ready_future<wal_read_reply::maybe>(
          std::move(maybe_buf));
      });
}

seastar::future<> wal_impl_with_cache::open() {
    LOG_INFO("starting: {}", opts);
    auto dir = opts.directory;
    return file_exists(dir)
      .then([dir](bool exists) {
        if (exists) { return seastar::make_ready_future<>(); }
        return seastar::make_directory(dir);
      })
      .then([this, dir] {
        return open_directory(dir).then([this](seastar::file f) {
          auto l = seastar::make_lw_shared<wal_file_name_mender>(std::move(f));
          return l->done().finally([l] {});
        });
      })
      .then([this] {
        return writer_->open().then([this] { return reader_->open(); });
      });
}

seastar::future<> wal_impl_with_cache::close() {
    LOG_INFO("stopping: {}", opts);
    return writer_->close().then([this] { return reader_->close(); });
}

}  // namespace smf
