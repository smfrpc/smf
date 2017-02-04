// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// third party
#include <core/sstring.hh>
#include <re2/re2.h>


namespace smf {
static const re2::RE2 kFileNameRE("[:a-zA-Z\\d]+_\\d+\\.wal");
struct wal_name_parser {
  bool operator()(const sstring &filename) {
    return re2::RE2::FullMatch(filename.c_str(), kFileNameRE);
  }
};


}  // namespace smf
