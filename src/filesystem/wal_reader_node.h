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
  struct wal_reader_node_stats {
    uint64_t total_reads{0};
    uint64_t total_bytes{0};
    uint64_t update_size_count{0};
  };

 public:
  wal_reader_node(uint64_t starting_file_epoch, seastar::sstring filename);
  ~wal_reader_node();

  const int64_t          starting_epoch;
  const seastar::sstring filename;

  /// \brief flushes the file before closing
  seastar::future<> close();
  seastar::future<> open();

  seastar::future<wal_read_reply> get(wal_read_request r);
  void update_file_size_by(uint64_t delta) {
    ++stats_.update_size_count;
    file_size_ += delta;
    io_->update_file_size_by(delta);
  }

  inline int64_t file_size() const { return file_size_; }
  inline int64_t ending_epoch() const { return starting_epoch + file_size_; }

 private:
  seastar::future<> open_node();

 private:
  std::unique_ptr<wal_clock_pro_cache> io_;
  uint64_t                             file_size_;
  wal_reader_node_stats                stats_{};
  seastar::metrics::metric_groups      metrics_{};
};

}  // namespace smf
