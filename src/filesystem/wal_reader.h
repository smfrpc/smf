// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <list>
#include <memory>
#include <utility>

#include <core/sstring.hh>

#include "filesystem/wal_file_walker.h"
#include "filesystem/wal_reader_node.h"
#include "filesystem/wal_requests.h"
#include "flatbuffers/wal_generated.h"

namespace smf {
class wal_reader;
struct wal_reader_visitor : wal_file_walker {
  wal_reader_visitor(wal_reader *r, seastar::file dir);
  seastar::future<> visit(seastar::directory_entry wal_file_entry) final;
  wal_reader *      reader;
};

/// \brief - given a starting directory and an epoch to read
/// it will iterate through the records on the file.
///
/// - in design
class wal_reader {
 public:
  explicit wal_reader(seastar::sstring workdir);
  wal_reader(wal_reader &&o) noexcept;
  ~wal_reader();
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_reader);

  seastar::future<> open();
  seastar::future<> close();
  /// brief - returns the next record in the log
  seastar::future<seastar::lw_shared_ptr<wal_read_reply>> get(
    wal_read_request req);

  const seastar::sstring work_directory;

 private:
  friend wal_reader_visitor;
  seastar::future<> monitor_files(seastar::directory_entry wal_file_entry);


  std::vector<std::unique_ptr<wal_reader_node>> nodes_;
  std::unique_ptr<wal_reader_visitor>           fs_observer_;
};

}  // namespace smf
