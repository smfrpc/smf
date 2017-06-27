// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <core/sstring.hh>
#include <core/fstream.hh>

#include "filesystem/wal_writer_file_lease.h"

namespace smf {

/// \brief changes the filename while it's being written to:
///   "locked:" + filename
/// upon successful closing of the stream, it will rename it to the correct
/// name. It is also open w/ exclusive acess. Any other thread accessing the
/// file will fail/throw. Caller must ensure that only one filename is
/// needed for writing at a time
/// see \ref wal_file_manager.h
///
/// Rejected Solutions: Keep a pointer to the file_manager.
///
/// having a pointer to the file manager introduces unnecessary complexity,
/// given that we have to monitor already the filesystem for files dropped
/// by other cores etc.
///
/// The tradeoff here is that since we change the filename, we rely on the
/// filesystem manger to "heal" the files. That is during startup,
/// rename all the files that start w/ locked: to remove the prefix.
///
class wal_writer_file_lease_impl : public virtual wal_writer_file_lease {
 public:
  wal_writer_file_lease_impl(seastar::sstring                    filename,
                             seastar::file_output_stream_options opts);
  ~wal_writer_file_lease_impl();
  seastar::future<> open() final;
  seastar::future<> close() final;
  seastar::future<> flush() final;
  seastar::future<> append(const char *buf, size_t n) final;
  static const seastar::sstring &lock_prefix();

 private:
  const seastar::sstring              original_name_;
  seastar::file_output_stream_options opts_;
  seastar::output_stream<char>        fstream_;
  bool                                closed_{false};
};

}  // namespace smf
