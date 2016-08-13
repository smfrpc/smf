#include <cstdlib>
#include <hdr/hdr_histogram.h>
#include <hdr/hdr_histogram_log.h>
#include "histogram.h"

namespace smf {
histogram::histogram() { init(); }
histogram::histogram(histogram &&o) noexcept : hist_(std::move(o.hist_)) {}
histogram::histogram(const struct hdr_histogram *copy) noexcept {
  init();
  ::hdr_add(hist_, copy);
}
void histogram::init() {
  ::hdr_init(1,                   // Minimum value
             INT64_C(3600000000), // Maximum value
             3,                   // Number of significant figures
             &hist_);             // Pointer to initialise
}

void histogram::record(const uint64_t &v) { ::hdr_record_value(hist_, v); }

void histogram::record_multiple_times(const uint64_t &v,
                                      const uint32_t &times) {
  ::hdr_record_values(hist_, v, times);
}

void histogram::record_corrected(const uint64_t &v, const uint64_t &interval) {
  ::hdr_record_corrected_value(hist_, v, interval);
}

size_t histogram::memory_size() { return ::hdr_get_memory_size(hist_); }

const struct hdr_histogram *histogram::get() const { return hist_; }

void histogram::operator+=(const histogram &o) { ::hdr_add(hist_, o.get()); }

std::unique_ptr<struct histogram_measure> histogram::auto_measure() {
  return std::make_unique<struct histogram_measure>(this);
}

void histogram::print(FILE *fp) const {
  assert(fp != nullptr);
  ::hdr_percentiles_print(hist_,
                          fp,       // File to write to
                          5,        // Granularity of printed values
                          1.0,      // Multiplier for results
                          CLASSIC); // Format CLASSIC/CSV supported.
}

histogram::~histogram() {
  if(hist_) {
    free(hist_);
    hist_ = nullptr;
  }
}
} // end namespace
