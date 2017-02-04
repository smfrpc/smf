// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_name_extractor_utils.h"
#include <re2/re2.h>

namespace smf {

uint64_t extract_epoch(const sstring &filename) {
  static const re2::RE2 kFileNameEpochExtractorRE("[:a-zA-Z\\d]+_(\\d+)\\.wal");
  uint64_t              retval = 0;
  re2::RE2::FullMatch(filename.c_str(), kFileNameEpochExtractorRE, &retval);
  return retval;
}


}  // namespace smf
