// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_writer_file_lease.h"

#include <core/file.hh>
#include <core/reactor.hh>

#include "platform/log.h"

namespace smf {

wal_writer_file_lease::wal_writer_file_lease(
  seastar::sstring filename, seastar::file_output_stream_options opts)
  : original_name_(filename), opts_(opts) {}
wal_writer_file_lease::~wal_writer_file_lease() {
  LOG_ERROR_IF(!closed_, "***DATA LOSS*** wal_file_lease was not closed. "
                         "Orignal name {}, current name: {}",
               original_name_, lock_prefix() + original_name_);
}

seastar::future<> wal_writer_file_lease::open() {
  auto name = lock_prefix() + original_name_;
  // the file should fail if it exists. It should not exist on disk, as
  // we'll truncate them
  auto flags = seastar::open_flags::rw | seastar::open_flags::create
               | seastar::open_flags::truncate | seastar::open_flags::exclusive;

  return seastar::open_file_dma(name, flags).then([this](seastar::file f) {
    fstream_ = make_file_output_stream(std::move(f), opts_);
    return seastar::make_ready_future<>();
  });
}
seastar::future<> wal_writer_file_lease::close() {
  closed_      = true;
  auto current = lock_prefix() + original_name_;
  auto real    = original_name_;

  // just like seastar, the user is responsible for flush()-ing the file stream
  // otherwise we endup double flushing on non-aligned pages
  return fstream_.close().then([real, current] {
    return seastar::rename_file(current, real)
      .handle_exception([real, current](auto ep) {
        LOG_ERROR("Could not rename: {} to {}. Error: {}", current, real, ep);
        return seastar::make_exception_future<>(ep);
      });
  });
}

seastar::future<> wal_writer_file_lease::flush() { return fstream_.flush(); }

seastar::future<> wal_writer_file_lease::append(const char *buf, size_t n) {
  // TODO(agallego): add seastar::temporary_buffer<char>
  // https://groups.google.com/forum/?hl=en#!topic/seastar-dev/SY-VG9xPVaY
  // buffers need to be aligned on the filesystem
  // so it's better if you let the underlying system copy
  // them - pending my fix to merge upstream
  return fstream_.write(buf, n);
}

const seastar::sstring &wal_writer_file_lease::lock_prefix() {
  static const seastar::sstring kCorePrefix = "locked:";
  return kCorePrefix;
}


}  // namespace smf
