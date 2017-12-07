// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// third party
#include <core/sstring.hh>

namespace smf {
struct wal_name_extractor_utils {
  /// \brief extract a uint64_t epoch out of a write ahead log name
  static uint64_t extract_epoch(const seastar::sstring &filename);
  /// \brief determines if is a wal_file or not
  static bool is_wal_file_name(const seastar::sstring &filename);
};

}  // namespace smf
