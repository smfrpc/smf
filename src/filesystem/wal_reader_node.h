#pragma once
// seastar
#include <core/fstream.hh>
// generated
#include "flatbuffers/wal_generated.h"
// smf
#include "filesystem/lazy_file.h"
#include "filesystem/wal_opts.h"
#include "filesystem/wal_requests.h"

namespace smf {
class wal_reader_node {
  public:
  wal_reader_node(uint64_t epoch, sstring filename, reader_stats *stats);
  ~wal_reader_node();

  const uint64_t starting_epoch;
  const sstring filename;

  /// \brief flushes the file before closing
  future<> close();
  future<> open();

  future<wal_opts::maybe_buffer> get(wal_read_request r);

  inline uint64_t file_size() { return io_->file_size; }
  inline uint64_t ending_epoch() const {
    return starting_epoch + io_->file_size;
  }

  private:
  reader_stats *rstats_;
  std::unique_ptr<lazy_file> io_;
};

} // namespace smf
