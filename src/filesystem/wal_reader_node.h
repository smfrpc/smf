// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <memory>
// seastar
#include <core/fstream.hh>
// generated
#include "flatbuffers/wal_generated.h"
// smf
#include "filesystem/wal_clock_pro_cache.h"
#include "filesystem/wal_opts.h"
#include "filesystem/wal_requests.h"

namespace smf {
class wal_reader_node {
 public:
  wal_reader_node(uint64_t epoch, sstring filename, reader_stats *stats);
  ~wal_reader_node();

  const int64_t starting_epoch;
  const sstring filename;

  /// \brief flushes the file before closing
  future<> close();
  future<> open();

  future<wal_read_reply::maybe> get(wal_read_request r);

  inline int64_t file_size() { return io_->file_size; }
  inline int64_t ending_epoch() const {
    return starting_epoch + io_->file_size;
  }

 private:
  reader_stats *                       rstats_;
  std::unique_ptr<wal_clock_pro_cache> io_;
};

}  // namespace smf
