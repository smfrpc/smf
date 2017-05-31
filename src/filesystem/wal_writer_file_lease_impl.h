// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include "filesystem/wal_writer_file_lease.h"
#include "platform/log.h"

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
                             seastar::file_output_stream_options opts)
    : original_name_(filename), opts_(opts) {}
  ~wal_writer_file_lease_impl() {
    LOG_ERROR_IF(!closed_, "***DATA LOSS*** wal_file_lease was not closed. "
                           "Orignal name {}, current name: {}",
                 original_name_, lock_prefix() + original_name_);
  }

  seastar::future<> open() final {
    auto name = lock_prefix() + original_name_;
    // the file should fail if it exists. It should not exist on disk, as
    // we'll truncate them
    auto flags = seastar::open_flags::rw | seastar::open_flags::create
                 | seastar::open_flags::truncate
                 | seastar::open_flags::exclusive;

    return seastar::open_file_dma(name, flags).then([this](seastar::file f) {
      fstream_ = make_file_output_stream(std::move(f), opts_);
      return seastar::make_ready_future<>();
    });
  }
  seastar::future<> close() final {
    closed_      = true;
    auto current = lock_prefix() + original_name_;
    auto real    = original_name_;
    return fstream_.flush()
      .then([this] { return fstream_.close(); })
      .then([real, current] { return seastar::rename_file(current, real); });
  }

  seastar::future<> flush() final { return fstream_.flush(); }

  seastar::future<> append(const char *buf, size_t n) final {
    // TODO(agallego): add seastar::temporary_buffer<char>
    // https://groups.google.com/forum/?hl=en#!topic/seastar-dev/SY-VG9xPVaY
    // buffers need to be aligned on the filesystem
    // so it's better if you let the underlying system copy
    // them - pending my fix to merge upstream
    return fstream_.write(buf, n);
  }

  static const seastar::sstring &lock_prefix() {
    static thread_local const seastar::sstring kCorePrefix = "locked:";
    return kCorePrefix;
  }

 private:
  const seastar::sstring              original_name_;
  seastar::file_output_stream_options opts_;
  seastar::output_stream<char>        fstream_;
  bool                                closed_{false};
};

}  // namespace smf
