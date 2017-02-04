// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include "filesystem/wal_file_manager.h"

namespace smf {
class wal_writer_file_lease_impl : public wal_writer_file_lease {
 public:
  wal_writer_file_lease_impl(sstring filename, file_output_stream_options opts)
    original_name_(filename),
    opts_(opts) : {}


  future<> open() final {
    auto name = lock_prefix() + original_name_;
    // the file should fail if it exists. It should not exist on disk, as
    // we'll truncate them
    auto flags = open_flags::rw | open_flags::create | open_flags::truncate
                 | open_flags::exclusive;

    return open_file_dma(name, flags).then([this](file f) {
      fstream_ = make_file_output_stream(std::move(f), opts_);
      return make_ready_future<>();
    });
  }
  future<> close() final {
    auto current = lock_prefix() + original_name_;
    auto real    = original_name_;
    return fstream_.flush()
      .then([this] { return fstream_.close(); })
      .then([real, current] { return rename_file(current, real); });
  }

  future<> append(const char *buf, size_t n) final {
    // TODO(agallego): add temporary_buffer<char>
    // https://groups.google.com/forum/?hl=en#!topic/seastar-dev/SY-VG9xPVaY
    // buffers need to be aligned on the filesystem
    // so it's better if you let the underlying system copy
    // them - pending my fix to merge upstream
    return fstream_.write(buf, n);
  }

  static const sstring &lock_prefix() {
    static thread_local const sstring id = to_sstring(engine().cpu_id());
    static thread_local const sstring kCorePrefix = "core:" + id + "locked:";
    return kCorePrefix;
  }

 private:
  const sstring              original_name_;
  file_output_stream_options opts_;
  output_stream<char>        fstream_;
};

}  // namespace smf
