// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// seastar
#include <core/fstream.hh>
#include <core/metrics_registration.hh>
#include <core/semaphore.hh>
#include <core/sstring.hh>
#include <core/timer.hh>
// generated
#include "flatbuffers/wal_generated.h"
// smf
#include "filesystem/wal_opts.h"
#include "filesystem/wal_requests.h"
#include "filesystem/wal_segment.h"
#include "filesystem/wal_write_projection.h"
#include "filesystem/wal_writer_utils.h"
#include "utils/time_utils.h"

namespace smf {
struct wal_writer_node_opts {
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_writer_node_opts);
  wal_writer_node_opts(const wal_opts &                  op,
                       const seastar::sstring &          writer_dir,
                       const seastar::io_priority_class &prio)
    : wopts(op), writer_directory(writer_dir), pclass(prio) {}

  wal_writer_node_opts(wal_writer_node_opts &&o) noexcept
    : wopts(o.wopts)
    , writer_directory(o.writer_directory)
    , pclass(o.pclass)
    , epoch(o.epoch)
    , write_concurrency(o.write_concurrency)
    , max_file_size(std::move(o.max_file_size)) {}

  const wal_opts &                  wopts;
  const seastar::sstring &          writer_directory;
  const seastar::io_priority_class &pclass;

  uint64_t       epoch                      = 0;
  const uint32_t write_concurrency          = 4; /*4 pages max concurrency*/
  const uint32_t write_file_in_memory_cache = 1024 * 1024; /*1MB*/
  const uint64_t max_file_size              = wal_file_size_aligned();
};

/// \brief - given a prefix and an epoch (monotinically increasing counter)
/// wal_writer_node will continue writing records in file_size multiples
/// which by default is 64MB. It will compress the buffers that are bigger
/// than min_compression_size
///
/// Note: the file closes happen concurrently, they simply get scheduled,
/// however, the fstream_.close() is called after the file has been flushed
/// to disk, so even during crash we are safe
///
class wal_writer_node {
 public:
  struct wal_writer_node_stats {
    uint64_t total_writes{0};
    uint64_t total_bytes{0};
    uint32_t total_rolls{0};
  };

 public:
  explicit wal_writer_node(wal_writer_node_opts &&opts);

  /// \brief writes the projection to disk ensuring file size capacity
  seastar::future<seastar::lw_shared_ptr<wal_write_reply>> append(
    const smf::wal::tx_put_partition_tuple *it);
  /// \brief flushes the file before closing
  seastar::future<> close();
  /// \brief opens the file w/ open_flags::rw | open_flags::create |
  ///                          open_flags::truncate
  /// the file should fail if it exists. It should not exist on disk, as
  /// we'll truncate them
  seastar::future<> open();

  ~wal_writer_node();

  uint64_t
  space_left() const {
    return opts_.max_file_size - current_size_;
  }
  uint64_t
  current_offset() const {
    return opts_.epoch + current_size_;
  }

 private:
  seastar::future<> rotate_fstream();
  /// \brief 0-copy append to buffer
  /// https://github.com/apache/kafka/blob/fb21209b5ad30001eeace56b3c8ab060e0ceb021/core/src/main/scala/kafka/log/Log.scala
  /// do append has a similar logic as the kafka log.
  /// effectively just check if there is enough space, if not rotate and then
  /// write.
  seastar::future<> do_append(const smf::wal::tx_put_binary_fragment *f);
  seastar::future<> disk_write(const smf::wal::tx_put_binary_fragment *f);


 private:
  wal_writer_node_opts opts_;
  uint64_t             current_size_ = 0;
  // the lease has to be a lw_shared_ptr because the object
  // may go immediately out of existence, before we get a chance to close the
  // file it needs to exist in the background fiber that closes the
  // underlying file
  //
  seastar::lw_shared_ptr<wal_segment> lease_ = nullptr;
  seastar::semaphore                  serialize_writes_{1};
  wal_writer_node_stats               stats_{};
  seastar::metrics::metric_groups     metrics_{};
  seastar::timer<>                    flush_timeout_;
};

}  // namespace smf
