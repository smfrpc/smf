// Copyright 2017 Alexander Gallego
//
#pragma once

#include <chrono>
#include <iostream>
#include <sstream>

#include "smf/histogram.h"
#include "smf/human_bytes.h"

namespace smf {

struct load_generator_duration {
  explicit load_generator_duration(uint64_t reqs) : num_of_req(reqs) {}
  load_generator_duration(load_generator_duration &&d) noexcept
    : num_of_req(std::move(d.num_of_req)), test_begin(std::move(d.test_begin)),
      test_end(std::move(d.test_end)), total_bytes(std::move(d.total_bytes)) {}

  uint64_t num_of_req;
  std::chrono::high_resolution_clock::time_point test_begin;
  std::chrono::high_resolution_clock::time_point test_end;

  uint64_t total_bytes{0};

  void
  begin() {
    test_begin = std::chrono::high_resolution_clock::now();
  }
  void
  end() {
    test_end = std::chrono::high_resolution_clock::now();
  }

  inline uint64_t
  duration_in_millis() const {
    namespace co = std::chrono;
    auto d = test_end - test_begin;
    return co::duration_cast<co::milliseconds>(d).count();
  }

  inline uint64_t
  qps() const {
    auto milli = duration_in_millis();
    const auto reqs = static_cast<double>(num_of_req);

    // some times the test run under 1 millisecond
    const auto safe_denom = std::max<uint64_t>(milli, 1);
    const auto denom = static_cast<double>(safe_denom);

    auto queries_per_milli = reqs / denom;

    return queries_per_milli * 1000.0;
  }
};

inline std::ostream &
operator<<(std::ostream &o, const smf::load_generator_duration &d) {
  o << "generator_duration={ test_duration= " << d.duration_in_millis()
    << "ms, qps=" << d.qps()
    << ", total_bytes=" << smf::human_bytes(d.total_bytes) << "("
    << d.total_bytes << ") }";
  return o;
}

}  // namespace smf
