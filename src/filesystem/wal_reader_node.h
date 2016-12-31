#pragma once
// seastar
#include <core/fstream.hh>
// generated
#include "flatbuffers/wal_generated.h"
// smf
#include "filesystem/lazy_file.h"
#include "filesystem/wal_opts.h"

namespace smf {
class wal_reader_node {
  public:
  // TOOD(agallego)
  // needs to extract startin offset from filename
  wal_reader_node(sstring _filename, reader_stats *s);
  /// \brief flushes the file before closing
  future<> close();
  future<> open();
  future<wal_opts::maybe_buffer> get(uint64_t epoch);
  ~wal_reader_node();
  const sstring filename;

  uint64_t starting_offset() const { return starting_offset_; }
  uint64_t file_size() { return io_->file_size; }

  private:
  reader_stats *rstats_;
  uint64_t starting_offset_;
  std::unique_ptr<lazy_file> io_;
};

} // namespace smf
