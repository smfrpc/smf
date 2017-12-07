// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_name_extractor_utils.h"

#include <re2/re2.h>

namespace smf {

uint64_t
wal_name_extractor_utils::extract_epoch(const seastar::sstring &filename) {
  static const re2::RE2 kEpochExtractor("(\\d+)\\.wal");
  uint64_t              retval = 0;
  re2::RE2::FullMatch(filename.c_str(), kEpochExtractor, &retval);
  return retval;
}
bool
wal_name_extractor_utils::is_wal_file_name(const seastar::sstring &filename) {
  static const re2::RE2 kFileNameRE("\\d+\\.wal");
  return re2::RE2::FullMatch(filename.c_str(), kFileNameRE);
}


}  // namespace smf
