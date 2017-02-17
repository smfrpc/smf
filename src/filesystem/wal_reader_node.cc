// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_reader_node.h"
#include <memory>
#include <utility>
// third party
#include <core/reactor.hh>
// smf
#include "platform/log.h"

namespace smf {
wal_reader_node::wal_reader_node(uint64_t      epoch,
                                 sstring       _filename,
                                 reader_stats *s)
  : starting_epoch(epoch), filename(_filename), rstats_(DTHROW_IFNULL(s)) {}
wal_reader_node::~wal_reader_node() {}

future<> wal_reader_node::close() { return io_->close(); }

future<> wal_reader_node::open() {
  return open_file_dma(filename, open_flags::ro).then([this](file f) {
    auto shared_f = make_lw_shared<file>(std::move(f));
    return shared_f->size().then([shared_f, this](uint64_t size) {
      io_ = std::make_unique<wal_clock_pro_cache>(shared_f, size);
      return make_ready_future<>();
    });
  });
}

future<wal_read_reply::maybe> wal_reader_node::get(wal_read_request r) {
  if (r.offset >= starting_epoch) {
    r.offset -= starting_epoch;
    r.size = std::min(r.size, ending_epoch());
    return io_->read(r).then([](auto buf) {
      return make_ready_future<wal_read_reply::maybe>(std::move(buf));
    });
  }
  return make_ready_future<wal_read_reply::maybe>();
}

}  // namespace smf
