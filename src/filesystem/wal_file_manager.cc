// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_file_manager.h"

namespace smf {
wal_file_manager::wal_file_manager() {}
wal_file_manager::~wal_file_manager() {}

std::unique_ptr<wal_writer_file_lease> wal_file_manager::get_file_lease(
  sstring filename, file_output_stream_options stream_otps) {}


void wal_file_manager::set_compaction_period(
  steady_clock_type::duration period = 100ms) {}

}  // namespace smf
