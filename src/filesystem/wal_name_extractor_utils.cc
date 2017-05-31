// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_name_extractor_utils.h"
#include <re2/re2.h>

namespace smf {

uint64_t wal_name_extractor_utils::extract_epoch(
  const seastar::sstring &filename) {
  static const re2::RE2 kEpochExtractor("(?i)[:a-z\\d]+:(\\d+)\\.wal");
  uint64_t              retval = 0;
  re2::RE2::FullMatch(filename.c_str(), kEpochExtractor, &retval);
  return retval;
}

uint64_t wal_name_extractor_utils::extract_core(
  const seastar::sstring &filename) {
  static const re2::RE2 kCoreNoExtractor(
    "(?i)[a-z:\\d]+[:a-z]+(\\d+):\\d+\\.wal");
  uint64_t retval = 0;
  re2::RE2::FullMatch(filename.c_str(), kCoreNoExtractor, &retval);
  return retval;
}

bool wal_name_extractor_utils::is_wal_file_name(
  const seastar::sstring &filename) {
  static const re2::RE2 kFileNameRE("(?i)[:a-z\\d]+:\\d+\\.wal");
  return re2::RE2::FullMatch(filename.c_str(), kFileNameRE);
}


seastar::sstring wal_name_extractor_utils::name_without_prefix(
  const seastar::sstring &s) {
  auto pos = wal_name_extractor_utils::locked_pos(s) + 7;
  return seastar::sstring(s.data() + pos, s.size() - pos);
}

size_t wal_name_extractor_utils::locked_pos(const seastar::sstring &s) {
  static const seastar::sstring kLockedPrefix = "locked:";
  return s.find(kLockedPrefix);
}

bool wal_name_extractor_utils::is_name_locked(const seastar::sstring &s) {
  return wal_name_extractor_utils::locked_pos(s) != seastar::sstring::npos;
}

}  // namespace smf
