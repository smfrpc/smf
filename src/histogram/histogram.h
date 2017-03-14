// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <hdr/hdr_histogram.h>
// c++ header
#include <cassert>
#include <chrono>
#include <memory>
#include <utility>

/// NOTE: This is not a seastar backed histogram
/// please link against the histogram_seastar_utils.h
/// the idea of this is to use with the wangle rpc as well

namespace smf {
class histogram;
struct histogram_measure;
}

namespace smf {
struct hist_t {
  hist_t() {
    ::hdr_init(1,                    // Minimum value
               INT64_C(3600000000),  // Maximum value
               3,                    // Number of significant figures
               &hist);               // Pointer to initialise
  }
  ~hist_t() { free(hist); }
  struct hdr_histogram *hist;
};

/// brief - simple wrapper for hdr_histogram_c project
///
class histogram {
 public:
  histogram();
  histogram(const histogram &o) = delete;
  explicit histogram(const struct hdr_histogram *copy) noexcept;
  histogram &operator+=(const histogram &o) noexcept;
  histogram &operator=(histogram &&o) noexcept;
  histogram &operator=(const histogram &o) noexcept;

  histogram(histogram &&o) noexcept;
  void record(const uint64_t &v);

  void record_multiple_times(const uint64_t &v, const uint32_t &times);
  void record_corrected(const uint64_t &v, const uint64_t &interval);
  int64_t value_at(double percentile) const;
  double stddev() const;
  double mean() const;
  size_t memory_size() const;

  const struct hdr_histogram *get() const;

  std::unique_ptr<struct histogram_measure> auto_measure();

  int print(FILE *fp) const;

  ~histogram();

 private:
  std::unique_ptr<hist_t> hist_ = std::make_unique<hist_t>();
};
std::ostream &operator<<(std::ostream &, const smf::histogram &h);

/// simple struct that records the measurement at the dtor
/// similar to boost_scope_exit;
struct histogram_measure {
  explicit histogram_measure(histogram *hist)
    : h(hist), begin_t(std::chrono::high_resolution_clock::now()) {
    assert(h != nullptr);
  }
  histogram_measure(const histogram_measure &o) = delete;
  histogram_measure(histogram_measure &&o) noexcept
    : h(o.h),
      begin_t(std::move(o.begin_t)) {}

  void set_trace(bool b) { trace_ = b; }

  ~histogram_measure() {
    if (h && trace_) {
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::high_resolution_clock::now() - begin_t)
                        .count();
      h->record(duration);
    }
  }

  bool       trace_ = true;
  histogram *h;

  std::chrono::high_resolution_clock::time_point begin_t;
};
}  // namespace smf
