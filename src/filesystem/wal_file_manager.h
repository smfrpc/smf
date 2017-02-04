// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

namespace smf {
// TODO(agallego)- could use a better name
class wal_writer_file_lease {
  virtual future<> close() = 0;
  virtual future<> open()  = 0;
  virtual future<> append(const char *buf, size_t n) = 0;
};

struct bad_wal_file_repair {
  future<> operator()(sstring name);
};

/// this is a point to put the compactions
/// directory creation and scannin ...
/// file healing

/// FIXME(agallego) - what we need to do is the file scannin of the
/// reading and the writing to happen in this class only
/// then have a class for subscriptions of functions... and do a
/// when_all_succeed()
/// needs to update seastar for the latest c-ares s

// WIP

class wal_file_manager {
 public:
  /// \brief changes the filename while it's being written to:
  ///   "current_" + filename
  /// upon successful closing of the stream, it will rename it to the correct
  /// name. It is also open w/ exclusive acess. Any other thread accessing the
  /// file will fail/throw. Caller must ensure that only one filename is
  /// needed for writing at a time
  ///
  std::unique_ptr<wal_writer_file_lease> get_file_lease(
    sstring filename, file_output_stream_options stream_otps);


  /// \brief enables the timer for compaction for the files
  /// managed by this core
  void enable_timer(steady_clock_type::time_point when);

 private:
};

}  // namespace smf
