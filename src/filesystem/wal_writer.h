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
  wal_writer(seastar::sstring _directory, writer_stats *s);
  wal_writer(const wal_writer &o) = delete;
  wal_writer(wal_writer &&o) noexcept;

  /// \brief opens the base directory and scans the files
  /// to determine last epoch written for this dir & prefix
  seastar::future<> open();
  /// \brief returns starting offset
  seastar::future<uint64_t> append(wal_write_request req);
  /// \brief writes a wal_write_request with an invalidation
  seastar::future<> invalidate(uint64_t epoch);
  /// \brief closes current file
  seastar::future<> close();

  /// \brief current working directory for wal files
  const seastar::sstring directory;

 private:
  seastar::future<> do_open();
  seastar::future<> open_empty_dir(seastar::sstring prefix);
  seastar::future<> open_non_empty_dir(seastar::sstring last_file,
                                       seastar::sstring prefix);

 private:
  writer_stats *                   wstats_;
  std::unique_ptr<wal_writer_node> writer_ = nullptr;
};

}  // namespace smf
