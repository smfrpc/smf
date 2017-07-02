// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <core/fstream.hh>
#include <core/sstring.hh>

namespace smf {

// TODO(agallego) - for historic reasons this is called a lease, but
// should be renamed


class wal_writer_file_lease {
 public:
  wal_writer_file_lease(seastar::sstring                    filename,
                        seastar::file_output_stream_options opts);
  ~wal_writer_file_lease();
  seastar::future<> open();
  seastar::future<> close();
  seastar::future<> flush();
  seastar::future<> append(const char *buf, size_t n);
  static const seastar::sstring &lock_prefix();

 private:
  const seastar::sstring              filename_;
  seastar::file_output_stream_options opts_;
  seastar::output_stream<char>        fstream_;
  bool                                closed_{false};
};

}  // namespace smf
