// Copyright 2017 Alexander Gallego
//
#pragma once

#include <chrono>

#include "histogram/histogram.h"

namespace smf {
namespace load_gen {

struct generator_duration {
  explicit generator_duration(uint64_t reqs) : num_of_req(reqs) {}
  generator_duration(generator_duration &&d) noexcept
    : num_of_req(std::move(d.num_of_req))
    , test_begin(std::move(d.test_begin))
    , test_end(std::move(d.test_end)) {}

  uint64_t                                       num_of_req;
  std::chrono::high_resolution_clock::time_point test_begin;
  std::chrono::high_resolution_clock::time_point test_end;

  void begin() { test_begin = std::chrono::high_resolution_clock::now(); }
  void end() { test_end = std::chrono::high_resolution_clock::now(); }

  inline uint64_t duration_in_millis() {
    namespace co = std::chrono;
    auto d       = test_end - test_begin;
    return co::duration_cast<co::milliseconds>(d).count();
  }

  inline uint64_t qps() {
    auto       milli = duration_in_millis();
    const auto reqs  = static_cast<double>(num_of_req);

    // some times the test run under 1 millisecond
    const auto safe_denom = std::max<uint64_t>(milli, 1);
    const auto denom      = static_cast<double>(safe_denom);

    auto queries_per_milli = reqs / denom;

    return queries_per_milli * 1000.0;
  }
};

}  // namespace load_gen
}  // namespace smf
