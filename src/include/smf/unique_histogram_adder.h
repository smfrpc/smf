// Copyright 2017 Alexander Gallego
//

#pragma once

#include <memory>

#include <seastar/core/future.hh>

#include <smf/histogram.h>

namespace smf {
class unique_histogram_adder {
 private:
  std::unique_ptr<smf::histogram> result_ = smf::histogram::make_unique();

 public:
  seastar::future<>
  operator()(std::unique_ptr<smf::histogram> value) {
    if (value) { *result_ += *value; }
    return seastar::make_ready_future<>();
  }
  seastar::future<>
  operator()(const smf::histogram *value) {
    if (value) { *result_ += *value; }
    return seastar::make_ready_future<>();
  }
  std::unique_ptr<smf::histogram>
  get() && {
    return std::move(result_);
  }
};

}  // namespace smf
