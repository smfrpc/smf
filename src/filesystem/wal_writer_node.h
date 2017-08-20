// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// seastar
#include <core/fstream.hh>
#include <core/semaphore.hh>
// generated
#include "flatbuffers/wal_generated.h"
// smf
#include "filesystem/wal_opts.h"
#include "filesystem/wal_requests.h"
#include "filesystem/wal_writer_file_lease.h"
#include "filesystem/wal_writer_utils.h"

namespace smf {
struct wal_writer_node_opts {
  seastar::sstring                    work_directory;
  seastar::sstring                    topic;
  uint32_t                            partition;
  uint64_t                            epoch = 0;
  seastar::file_output_stream_options fstream{
    // These are the seastar defaults
    //
    // unsigned buffer_size = 8192;
    // unsigned preallocation_size = 1024*1024; // 1MB
    // unsigned write_behind = 1; ///< Number of buffers to write in parallel
  };

  // This no longer makes sense. this makes sense in the projection
  const uint64_t min_compression_size = 512;
  const uint64_t file_size            = wal_file_size_aligned();
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
    // add more metrics!
  };

 public:
  explicit wal_writer_node(wal_writer_node_opts &&opts);

  /// \brief writes the projection to disk ensuring file size capacity
  seastar::future<wal_write_reply> append(
    seastar::lw_shared_ptr<wal_write_projection> req);
  /// \brief flushes the file before closing
  seastar::future<> close();
  /// \brief opens the file w/ open_flags::rw | open_flags::create |
  ///                          open_flags::truncate | open_flags::exclusive
  /// the file should fail if it exists. It should not exist on disk, as
  /// we'll truncate them
  seastar::future<> open();

  ~wal_writer_node();

  inline uint64_t space_left() const { return current_size_ - opts_.file_size; }
  inline uint64_t min_entry_size() const {
    return opts_.min_compression_size + sizeof(fbs::wal::wal_header);
  }

 private:
  seastar::future<> rotate_fstream();
  /// \brief 0-copy append to buffer
  seastar::future<> do_append(wal_write_request);
  seastar::future<> do_append_with_flags(wal_write_request,
                                         fbs::wal::wal_entry_flags);
  seastar::future<> do_append_with_header(fbs::wal::wal_header,
                                          wal_write_request);

 private:
  wal_writer_node_opts opts_;
  uint64_t             current_size_ = 0;
  // the lease has to be a lw_shared_ptr because the object
  // may go immediately out of existence, before we get a chance to close the
  // file it needs to exist in the background fiber that closes the
  // underlying file
  //
  seastar::lw_shared_ptr<wal_writer_file_lease> lease_ = nullptr;
  seastar::semaphore                            serialize_writes_{1};
  wal_writer_node_stats                         stats_{};
  seastar::metrics::metric_groups               metrics_{};
};

}  // namespace smf
