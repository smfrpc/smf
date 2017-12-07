// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <memory>
// seastar
#include <core/fstream.hh>
#include <core/sstring.hh>
#include <core/metrics_registration.hh>

// generated
#include "flatbuffers/wal_generated.h"
// smf
#include "filesystem/wal_opts.h"
#include "filesystem/wal_requests.h"

namespace smf {
class wal_reader_node {
 public:
  struct wal_reader_node_stats {
    uint64_t total_reads{0};
    uint64_t total_bytes{0};
  };

 public:
  wal_reader_node(uint64_t starting_file_epoch, seastar::sstring filename);
  ~wal_reader_node();

  const uint64_t         starting_epoch;
  const seastar::sstring filename;

  /// \brief flushes the file before closing
  seastar::future<> close();
  seastar::future<> open();

  seastar::future<> get(seastar::lw_shared_ptr<wal_read_reply> retval,
                        wal_read_request                       r);

  inline uint64_t
  file_size() const {
    return file_size_;
  }
  inline uint64_t
  ending_epoch() const {
    return starting_epoch + file_size_;
  }

 private:
  seastar::future<> open_node();
  /// \brief - return buffer for offset with size
  seastar::future<> do_read(seastar::lw_shared_ptr<wal_read_reply> retval,
                            wal_read_request                       r);

  /// \brief - returns a temporary buffer of size. similar to
  /// isotream.read_exactly()
  /// different than a wal_read_request for arguments since it returns **one**
  /// temp buffer
  ///
  seastar::future<seastar::temporary_buffer<char>> read_exactly(
    const uint64_t &                  offset,
    const uint64_t &                  size,
    const seastar::io_priority_class &pc);


 private:
  seastar::lw_shared_ptr<seastar::file> file_ = nullptr;
  uint64_t                              file_size_{0};
  /// \brief file_size_ / disk_dma_alignment
  uint64_t number_of_pages_{0};
  /// \brief hash of inode (st_ino) and device id (st_dev)
  uint64_t                        global_file_id_{0};
  wal_reader_node_stats           stats_{};
  seastar::metrics::metric_groups metrics_{};
};

}  // namespace smf
