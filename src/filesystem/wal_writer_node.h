// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// seastar
#include <core/fstream.hh>
// generated
#include "flatbuffers/wal_generated.h"
// smf
#include "filesystem/wal_opts.h"
#include "filesystem/wal_requests.h"
#include "filesystem/wal_writer_file_lease.h"
#include "filesystem/wal_writer_utils.h"

namespace smf {
// TODO(agallego) - use the stats internally now that you have them
struct wal_writer_node_opts {
  writer_stats * wstats;
  sstring        prefix;
  uint64_t       epoch                = 0;
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
  explicit wal_writer_node(wal_writer_node_opts &&opts);
  /// \brief 0-copy append to buffer
  /// \return the starting offset on file for this put
  ///
  /// breaks request into largest chunks possible for remaining space
  /// writing at most opts.file_size on disk at each step.
  /// once ALL chunks have been written, the original req is deallocatead
  /// internally, we call temporary_buffer<T>::share(i,j) to create shallow
  /// copies of the data deferring the deleter() function to deallocate buffer
  ///
  /// This is a recursive function
  ///
  future<uint64_t> append(wal_write_request req);
  /// \brief flushes the file before closing
  future<> close();
  /// \brief opens the file w/ open_flags::rw | open_flags::create |
  ///                          open_flags::truncate | open_flags::exclusive
  /// the file should fail if it exists. It should not exist on disk, as
  /// we'll truncate them
  future<> open();

  ~wal_writer_node();

  inline uint64_t space_left() const { return current_size_ - opts_.file_size; }
  inline uint64_t min_entry_size() const {
    return opts_.min_compression_size + sizeof(fbs::wal::wal_header);
  }

 private:
  future<> rotate_fstream();
  /// \brief 0-copy append to buffer
  future<> do_append(wal_write_request);
  future<> do_append_with_flags(wal_write_request, fbs::wal::wal_entry_flags);
  future<> do_append_with_header(fbs::wal::wal_header, wal_write_request);
  future<> pad_end_of_file();

 private:
  wal_writer_node_opts                   opts_;
  uint64_t                               current_size_ = 0;
  std::unique_ptr<wal_writer_file_lease> lease_        = nullptr;
};

}  // namespace smf
