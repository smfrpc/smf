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

seastar::future<> wal_reader_node::close() {
  if (io_) { return io_->close(); }
  return seastar::make_ready_future<>();
}

seastar::future<> wal_reader_node::open_node() {
  return seastar::open_file_dma(filename, seastar::open_flags::ro)
    .then([this](seastar::file ff) {
      auto f = seastar::make_lw_shared<seastar::file>(std::move(ff));
      io_    = std::make_unique<wal_clock_pro_cache>(f, file_size_);
      return seastar::make_ready_future<>();
    });
}
  // need to copy the logic from cache
seastar::future<wal_read_reply> wal_reader_node::get(
  wal_read_request r) {
  if (SMF_UNLIKELY(io_ == nullptr)) {
    return open_node().then([this, r = std::move(r)] { return this->get(r); });
  }
  if (r.offset >= starting_epoch) {
    r.offset -= starting_epoch;
    // this should substract the difference.
    // or rather

    ;;; foo-bar-break();//ugh i'm bored.....

    r.size = std::min(r.size, ending_epoch());
    return io_->read(r).then([](auto buf) {
      return seastar::make_ready_future<wal_read_reply>(std::move(buf));
    });
  }
  return seastar::make_ready_future<wal_read_reply>();
}

seastar::future<> wal_reader_node::open() {
  return seastar::file_size(filename).then(
    [this](auto size) { file_size_ = size; });
}

}  // namespace smf
