// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <cstdio>
// seastar
#include <core/fstream.hh>
// smf
#include "histogram.h"


namespace smf {
struct histogram_seastar_utils {
  static future<temporary_buffer<char>> print_histogram(histogram h);
  static future<> write_histogram(sstring filename, histogram h);
  static void sync_write_histogram(const sstring &filename, histogram *h);
};

}  // namespace smf
