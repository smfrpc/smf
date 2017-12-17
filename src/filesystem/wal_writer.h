// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <memory>
// third party
#include <core/sstring.hh>
// smf
#include "filesystem/wal_requests.h"
#include "filesystem/wal_writer_node.h"
#include "platform/macros.h"

namespace smf {
class wal_writer {
 public:
  wal_writer(seastar::sstring workdir);
  wal_writer(wal_writer &&o) noexcept;
  ~wal_writer() {}

  SMF_DISALLOW_COPY_AND_ASSIGN(wal_writer);

  /// \brief opens the base directory and scans the files
  /// to determine last epoch written for this dir & prefix
  seastar::future<> open();
  /// \brief returns starting offset
  seastar::future<wal_write_reply> append(
    seastar::lw_shared_ptr<wal_write_projection> projection);
  /// \brief closes current file
  seastar::future<> close();

  const seastar::sstring work_directory;

 private:
  wal_writer_node_opts default_writer_opts() const;

  seastar::future<> open_empty_dir();
  seastar::future<> open_non_empty_dir(seastar::sstring last_file);


 private:
  std::unique_ptr<wal_writer_node> writer_ = nullptr;
};

}  // namespace smf
