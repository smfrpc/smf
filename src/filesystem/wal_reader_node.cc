// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_reader_node.h"

#include <memory>
#include <system_error>
#include <utility>
// third party
#include <core/metrics.hh>
#include <core/reactor.hh>
#include <core/seastar.hh>
// smf
#include "platform/log.h"
#include "platform/macros.h"

namespace smf {
wal_reader_node::wal_reader_node(uint64_t epoch, seastar::sstring _filename)
  // needed signed for comparisons
  : starting_epoch(static_cast<int64_t>(epoch)),
    filename(_filename) {
  namespace sm = seastar::metrics;
  metrics_.add_group(
    "smf::wal_reader_node::" + filename,
    {sm::make_derive("bytes_read", stats_.total_bytes,
                     sm::description("bytes read from disk")),
     sm::make_derive("total_reads", stats_.total_reads,
                     sm::description("Number of read ops")),
     sm::make_derive("update_size_count", stats_.update_size_count,
                     sm::description("Number of times file was expanded"))});
}
wal_reader_node::~wal_reader_node() {}

seastar::future<> wal_reader_node::close() { return io_->close(); }

seastar::future<> wal_reader_node::open_node() {
  return seastar::open_file_dma(filename, seastar::open_flags::ro)
    .then([this](seastar::file ff) {
      auto f = seastar::make_lw_shared<seastar::file>(std::move(ff));
      io_    = std::make_unique<wal_clock_pro_cache>(f, file_size_);
      return seastar::make_ready_future<>();
    });
}

seastar::future<wal_read_reply> wal_reader_node::get(wal_read_request r) {
  auto offset = r.req->offset();
  if (offset >= static_cast<uint64_t>(starting_epoch)
      && offset <= static_cast<uint64_t>(ending_epoch())) {
    return io_->read(r, starting_epoch);
  }
  return seastar::make_ready_future<wal_read_reply>();
}

seastar::future<> wal_reader_node::open() {
  return seastar::file_size(filename)
    .then([this](auto size) { file_size_ = size; })
    .then([this] { return open_node(); });
}

}  // namespace smf
