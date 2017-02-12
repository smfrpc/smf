// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_name_extractor_utils.h"
#include <re2/re2.h>

namespace smf {

uint64_t wal_name_extractor_utils::extract_epoch(const sstring &filename) {
  static const re2::RE2 kEpochExtractor("(?i)[:a-z\\d]+:(\\d+)\\.wal");
  uint64_t              retval = 0;
  re2::RE2::FullMatch(filename.c_str(), kEpochExtractor, &retval);
  return retval;
}

uint64_t wal_name_extractor_utils::extract_core(const sstring &filename) {
  static const re2::RE2 kCoreNoExtractor(
    "(?i)[a-z:\\d]+[:a-z]+(\\d+):\\d+\\.wal");
  uint64_t retval = 0;
  re2::RE2::FullMatch(filename.c_str(), kCoreNoExtractor, &retval);
  return retval;
}

bool wal_name_extractor_utils::is_wal_file_name(const sstring &filename) {
  static const re2::RE2 kFileNameRE("(?i)[:a-z\\d]+:\\d+\\.wal");
  return re2::RE2::FullMatch(filename.c_str(), kFileNameRE);
}


sstring wal_name_extractor_utils::name_without_prefix(const sstring &s) {
  auto pos = wal_name_extractor_utils::locked_pos(s) + 7;
  return sstring(s.data() + pos, s.size() - pos);
}

size_t wal_name_extractor_utils::locked_pos(const sstring &s) {
  static const sstring kLockedPrefix = "locked:";
  return s.find(kLockedPrefix);
}

bool wal_name_extractor_utils::is_name_locked(const sstring &s) {
  return wal_name_extractor_utils::locked_pos(s) != sstring::npos;
}

}  // namespace smf
