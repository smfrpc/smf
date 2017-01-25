// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <memory>
// third party
#include <core/sstring.hh>
// smf
#include "filesystem/wal_requests.h"
#include "filesystem/wal_writer_node.h"

namespace smf {
class wal_writer {
 public:
  wal_writer(sstring _directory, writer_stats *s);
  wal_writer(const wal_writer &o) = delete;
  wal_writer(wal_writer &&o) noexcept;

  /// \brief opens the base directory and scans the files
  /// to determine last epoch written for this dir & prefix
  future<> open();
  /// \brief returns starting offset
  future<uint64_t> append(wal_write_request req);
  /// \brief writes a wal_write_request with an invalidation
  future<> invalidate(uint64_t epoch);
  /// \brief closes current file
  future<> close();

  /// \brief current working directory for wal files
  const sstring directory;

 private:
  future<> do_open();
  future<> open_empty_dir(sstring prefix);
  future<> open_non_empty_dir(sstring last_file, sstring prefix);

 private:
  writer_stats *                   wstats_;
  std::unique_ptr<wal_writer_node> writer_ = nullptr;
};

}  // namespace smf
