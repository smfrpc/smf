// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_writer_file_lease_impl.h"

#include <core/file.hh>
#include <core/reactor.hh>

#include "platform/log.h"

namespace smf {

wal_writer_file_lease_impl::wal_writer_file_lease_impl(
  seastar::sstring filename, seastar::file_output_stream_options opts)
  : original_name_(filename), opts_(opts) {}
wal_writer_file_lease_impl::~wal_writer_file_lease_impl() {
  LOG_ERROR_IF(!closed_, "***DATA LOSS*** wal_file_lease was not closed. "
                         "Orignal name {}, current name: {}",
               original_name_, lock_prefix() + original_name_);
}

seastar::future<> wal_writer_file_lease_impl::open() {
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
seastar::future<> wal_writer_file_lease_impl::close() {
  closed_      = true;
  auto current = lock_prefix() + original_name_;
  auto real    = original_name_;
  return fstream_.flush()
    .then([this] { return fstream_.close(); })
    .then([real, current] { return seastar::rename_file(current, real); });
}

seastar::future<> wal_writer_file_lease_impl::flush() {
  return fstream_.flush();
}

seastar::future<> wal_writer_file_lease_impl::append(const char *buf,
                                                     size_t      n) {
  // TODO(agallego): add seastar::temporary_buffer<char>
  // https://groups.google.com/forum/?hl=en#!topic/seastar-dev/SY-VG9xPVaY
  // buffers need to be aligned on the filesystem
  // so it's better if you let the underlying system copy
  // them - pending my fix to merge upstream
  return fstream_.write(buf, n);
}

const seastar::sstring &wal_writer_file_lease_impl::lock_prefix() {
  static const seastar::sstring kCorePrefix = "locked:";
  return kCorePrefix;
}


}  // namespace smf
