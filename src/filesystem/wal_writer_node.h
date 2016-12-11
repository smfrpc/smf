#pragma once
// seastar
#include <core/fstream.hh>
// generated
#include "flatbuffers/wal_generated.h"
// smf
#include "filesystem/wal_writer_utils.h"
#include "flatbuffers/wal_generated.h"

namespace smf {
//(TODO) missing stats & histograms

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
  wal_writer_node(sstring prefix,
                  uint64_t epoch = 0,
                  uint64_t min_compression_size = 512,
                  const uint64_t file_size = wal_file_size_aligned());

  /// \brief 0-copy append to buffer
  future<> append(temporary_buffer<char> &&buf);

  /// \brief flushes the file before closing
  future<> close();

  /// \brief opens the file w/ open_flags::rw | open_flags::create |
  ///                          open_flags::truncate | open_flags::exclusive
  /// the file should fail if it exists. It should not exist on disk, as
  /// we'll truncate them
  future<> open();

  ~wal_writer_node();

  inline uint64_t space_left() const { return current_size_ - max_size; }
  inline uint64_t min_entry_size() const {
    return min_compression_size + sizeof(fbs::wal::wal_header);
  }
  const sstring prefix_name;
  const uint64_t max_size;
  const uint64_t min_compression_size;

  private:
  future<> rotate_fstream();
  /// \brief 0-copy append to buffer
  future<> do_append(temporary_buffer<char> &&, fbs::wal::wal_entry_flags);
  future<> do_append_with_header(fbs::wal::wal_header h,
                                 temporary_buffer<char> &&buf);

  private:
  uint64_t epoch_;
  output_stream<char> fstream_;
  file_output_stream_options opts_;
  uint64_t current_size_ = 0;
  bool closed_ = false;
};

} // namespace smf
