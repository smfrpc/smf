// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// third party
#include <core/sstring.hh>
#include <re2/re2.h>

namespace smf {

inline uint64_t extract_epoch(const sstring &filename) {
  static const re2::RE2 kFileNameEpochExtractorRE("[a-zA-Z\\d]+_(\\d+)\\.wal");
  uint64_t              retval = 0;
  re2::RE2::FullMatch(filename.c_str(), kFileNameEpochExtractorRE, &retval);
  return retval;
}


}  // namespace smf
