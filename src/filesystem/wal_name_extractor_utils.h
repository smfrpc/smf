// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// third party
#include <core/sstring.hh>

namespace smf {
struct wal_name_extractor_utils {
  /// \brief extract a uint64_t epoch out of a write ahead log name
  /// "1234:0:<epoch>...wal"
  static uint64_t extract_epoch(const seastar::sstring &filename);
  /// \brief extract a uint64_t core out of a write ahead log name
  /// "<microsec>:<core>:<epoch>.wal" - 1234:0:0.wal -> core 0
  static uint64_t extract_core(const seastar::sstring &filename);
  /// \brief determines if is a wal_file or not
  /// "<microsec>:<core>:<epoch>.wal" - 1234:0:0.wal -> core 0
  static bool is_wal_file_name(const seastar::sstring &filename);

  /// \brief return name of \s without the locked prefix.
  /// i.e.: without locked:1234:0:0.wal -> 1234:0:0.wal
  static seastar::sstring name_without_prefix(const seastar::sstring &s);

  /// \brief return the position in the string that is locked
  static size_t locked_pos(const seastar::sstring &s);

  /// \brief determine if the name of the file is locked via
  /// i.e.: `locked:smf:0:0.wal`
  static bool is_name_locked(const seastar::sstring &s);
};

}  // namespace smf
