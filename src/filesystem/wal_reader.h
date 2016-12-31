#pragma once
// generated
#include "flatbuffers/wal_generated.h"
// smf
#include "filesystem/wal.h"
#include "filesystem/wal_file_walker.h"
#include "filesystem/wal_head_file_functor.h"
#include "filesystem/wal_reader_offset_range.h"
#include "filesystem/wal_requests.h"

namespace smf {
class wal_reader;
struct wal_reader_visitor : wal_file_walker {
  wal_reader_visitor(wal_reader *r, file dir, sstring prefix = "smf");
  virtual future<> visit(directory_entry wal_file_entry) override final;
  wal_reader *reader;
};

/// \brief - given a starting directory and an epoch to read
/// it will iterate through the records on the file.
///
/// - in design
class wal_reader {
  public:
  wal_reader(sstring _dir, reader_stats *s);
  wal_reader(wal_reader &&o) noexcept;
  ~wal_reader();
  future<> open();

  /// brief - returns the next record in the log
  future<wal_opts::maybe_buffer> get(wal_read_request req);

  const sstring directory;

  private:
  friend wal_reader_visitor;
  future<> monitor_files(directory_entry wal_file_entry);

  reader_stats *rstats_;
  std::vector<wal_reader_offset_range> readers_;
  std::unique_ptr<wal_reader_visitor> fs_observer_;
};

} // namespace smf
