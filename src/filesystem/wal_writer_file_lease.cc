// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_writer_file_lease.h"

#include <core/file.hh>
#include <core/reactor.hh>

#include "platform/log.h"

namespace smf {

wal_writer_file_lease::wal_writer_file_lease(
  seastar::sstring filename, seastar::file_output_stream_options opts)
  : filename_(filename), opts_(opts) {}

wal_writer_file_lease::~wal_writer_file_lease() {
  LOG_ERROR_IF(!closed_, "***DATA LOSS*** wal_file_lease was not closed. {}",
               filename_);
}

seastar::future<> wal_writer_file_lease::open() {
  return file_exists(filename_).then([this](bool filename_exists) {
    LOG_THROW_IF(filename_exists,
                 "File: {} already exists. Cannot re-open for writes",
                 filename_);
    auto flags = seastar::open_flags::rw | seastar::open_flags::create
                 | seastar::open_flags::truncate;
    return seastar::open_file_dma(filename_, flags)
      .then([this](seastar::file f) {
        fstream_ = make_file_output_stream(std::move(f), opts_);
        return seastar::make_ready_future<>();
      });
  });
}
seastar::future<> wal_writer_file_lease::close() {
  DLOG_THROW_IF(closed_, "File lease already closed. Bug: {}", filename_);
  closed_ = true;
  return fstream_.close();
}

seastar::future<> wal_writer_file_lease::flush() { return fstream_.flush(); }

seastar::future<> wal_writer_file_lease::append(const char *buf, size_t n) {
  // https://groups.google.com/forum/?hl=en#!topic/seastar-dev/SY-VG9xPVaY
  // buffers need to be aligned on the filesystem
  // so it's better if you let the underlying system copy them
  return fstream_.write(buf, n);
}

}  // namespace smf
