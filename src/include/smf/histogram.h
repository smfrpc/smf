// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <cassert>
#include <chrono>
#include <memory>
#include <utility>

#include <hdr_histogram.h>
#include <seastar/core/metrics_types.hh>
#include <seastar/core/shared_ptr.hh>

#include "smf/macros.h"

namespace smf {
class histogram;
struct histogram_measure;
}  // namespace smf

namespace smf {
// 1 hour in microsecs - max value
static constexpr const int64_t kDefaultHistogramMaxValue = 3600000000;

// VERY Expensive object. At this granularity is about 185KB
// per instance
struct hist_t {
  explicit hist_t(int64_t max_value) {
    ::hdr_init(1,          // 1 microsec - minimum value
               max_value,  // 1 hour in microsecs - max value
               3,          // Number of significant figures
               &hist);     // Pointer to initialize
  }

  hist_t(hist_t &&o) noexcept : hist(std::move(o.hist)) {}

  SMF_DISALLOW_COPY_AND_ASSIGN(hist_t);

  ~hist_t() {
    if (hist) {
      hdr_close(hist);
      hist = nullptr;
    }
  }
  hdr_histogram *hist = nullptr;
  uint64_t sample_count = 0;
  uint64_t sample_sum = 0;
};

/// brief - simple wrapper for hdr_histogram_c project
///
class histogram final : public seastar::enable_lw_shared_from_this<histogram> {
 public:
  static seastar::lw_shared_ptr<histogram>
  make_lw_shared(int64_t max_value = kDefaultHistogramMaxValue);

  static std::unique_ptr<histogram>
  make_unique(int64_t max_value = kDefaultHistogramMaxValue);

  SMF_DISALLOW_COPY_AND_ASSIGN(histogram);

  histogram(histogram &&o) noexcept;

  histogram &operator=(histogram &&o) noexcept;
  histogram &operator+=(const histogram &o);
  histogram &operator+=(const hist_t *o);

  void record(const uint64_t &v);

  void record_multiple_times(const uint64_t &v, const uint32_t &times);
  void record_corrected(const uint64_t &v, const uint64_t &interval);
  int64_t value_at(double percentile) const;
  double stddev() const;
  double mean() const;
  size_t memory_size() const;

  hdr_histogram *get();

  std::unique_ptr<histogram_measure> auto_measure();

  int print(FILE *fp) const;

  seastar::metrics::histogram seastar_histogram_logform() const;

  ~histogram();

 private:
  explicit histogram(int64_t max_value);
  friend seastar::lw_shared_ptr<histogram>;

 private:
  std::unique_ptr<hist_t> hist_;
};
/// simple struct that records the measurement at the dtor
/// similar to boost_scope_exit;
struct histogram_measure {
  explicit histogram_measure(seastar::lw_shared_ptr<histogram> ptr)
    : h(ptr), begin_t(std::chrono::high_resolution_clock::now()) {}

  SMF_DISALLOW_COPY_AND_ASSIGN(histogram_measure);

  histogram_measure(histogram_measure &&o) noexcept
    : trace_(o.trace_), h(std::move(o.h)), begin_t(o.begin_t) {}

  void
  set_trace(bool b) {
    trace_ = b;
  }

  ~histogram_measure() {
    if (h && trace_) {
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::high_resolution_clock::now() - begin_t)
                        .count();
      h->record(duration);
    }
  }

  bool trace_ = true;
  seastar::lw_shared_ptr<histogram> h = nullptr;
  std::chrono::high_resolution_clock::time_point begin_t;
};
}  // namespace smf

inline std::ostream &
operator<<(std::ostream &o, const smf::histogram &h) {
  o << "smf::histogram={p50=" << h.value_at(.5) << "μs,p99=" << h.value_at(.99)
    << "μs,p999=" << h.value_at(.999) << "μs}";
  return o;
};
