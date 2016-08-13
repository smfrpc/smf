#pragma once
#include <chrono>
#include <memory>
#include <cassert>
#include <hdr/hdr_histogram.h>

namespace smf {
class histogram;
struct histogram_measure;
}

namespace smf {
/// brief - simple wrapper for hdr_histogram_c project
///
class histogram {
  public:
  histogram();
  histogram(const histogram &o) = delete;
  histogram(const struct hdr_histogram *copy) noexcept;
  histogram(histogram &&o) noexcept;

  void operator+=(const histogram &o);
  void record(const uint64_t &v);
  void record_multiple_times(const uint64_t &v, const uint32_t &times);
  void record_corrected(const uint64_t &v, const uint64_t &interval);
  size_t memory_size();
  const struct hdr_histogram *get() const;

  std::unique_ptr<struct histogram_measure> auto_measure();

  void print(FILE *fp) const;

  ~histogram();

  private:
  void init();
  struct hdr_histogram *hist_ = nullptr;
};

/// simple struct that records the measurement at the dtor
/// similar to boost_scope_exit;
struct histogram_measure {
  histogram_measure(histogram *hist)
    : h(hist), begin_t(std::chrono::high_resolution_clock::now()) {
    assert(h != nullptr);
  }
  histogram_measure(const histogram_measure &o) = delete;
  histogram_measure(histogram_measure &&o) noexcept
    : h(o.h),
      begin_t(std::move(o.begin_t)) {}

  void set_trace(bool b) { trace_ = b; }
  ~histogram_measure() {
    if(h && trace_) {
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::high_resolution_clock::now() - begin_t)
                        .count();
      h->record(duration);
    }
  }
  bool trace_ = true;
  histogram *h;
  std::chrono::high_resolution_clock::time_point begin_t;
};
}
