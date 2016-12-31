#include "histogram.h"
#include <cstdlib>
#include <hdr/hdr_histogram.h>
#include <hdr/hdr_histogram_log.h>

namespace smf {
histogram::histogram() {}
histogram::histogram(histogram &&o) noexcept : hist_(std::move(o.hist_)) {}
histogram::histogram(const struct hdr_histogram *copy) noexcept {
  ::hdr_add(hist_->hist, copy);
}

histogram &histogram::operator+=(const histogram &o) noexcept {
  ::hdr_add(hist_->hist, o.get());
  return *this;
}

histogram &histogram::operator=(histogram &&o) noexcept {
  hist_ = std::move(o.hist_);
  return *this;
}

histogram &histogram::operator=(const histogram &o) noexcept {
  *this += o;
  return *this;
}
void histogram::record(const uint64_t &v) {
  ::hdr_record_value(hist_->hist, v);
}

void histogram::record_multiple_times(const uint64_t &v,
                                      const uint32_t &times) {
  ::hdr_record_values(hist_->hist, v, times);
}

void histogram::record_corrected(const uint64_t &v, const uint64_t &interval) {
  ::hdr_record_corrected_value(hist_->hist, v, interval);
}

size_t histogram::memory_size() { return ::hdr_get_memory_size(hist_->hist); }

const struct hdr_histogram *histogram::get() const { return hist_->hist; }

std::unique_ptr<struct histogram_measure> histogram::auto_measure() {
  return std::make_unique<struct histogram_measure>(this);
}

int histogram::print(FILE *fp) const {
  assert(fp != nullptr);
  return ::hdr_percentiles_print(hist_->hist,
                                 fp,       // File to write to
                                 5,        // Granularity of printed values
                                 1.0,      // Multiplier for results
                                 CLASSIC); // Format CLASSIC/CSV supported.
}

histogram::~histogram() {}


} // end namespace
