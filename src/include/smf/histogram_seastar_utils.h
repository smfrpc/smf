// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <cstdio>

#include <seastar/core/fstream.hh>

#include "smf/histogram.h"

namespace smf {
struct histogram_seastar_utils {
  static seastar::future<seastar::temporary_buffer<char>>
  print_histogram(histogram *h);
  inline static seastar::future<>
  write(seastar::sstring filename, std::unique_ptr<histogram> h) {
    auto p = h.get();
    return write_histogram(std::move(filename), p).finally([hh = std::move(h)] {
    });
  }
  inline static seastar::future<>
  write(seastar::sstring filename, seastar::lw_shared_ptr<histogram> h) {
    return write_histogram(std::move(filename), h.get()).finally([h] {});
  }
  static seastar::future<> write_histogram(seastar::sstring filename,
                                           histogram *h);
};

}  // namespace smf
