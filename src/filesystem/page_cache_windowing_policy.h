// Copyright 2018 Alexander Gallego
//

#pragma once

namespace smf {
struct page_cache_windowing_policy {
  // called during every page_fault
  virtual float
  grow_by() {
    return 1.0;
  }
  // called when we need to decrease the mem
  virtual float
  shrink_by(const uint64_t &min_reserved, const uint64_t &max_capacity) {
    return 0.25;
  }
  ~page_cache_windowing_policy() = default;
};

}  // namespace smf
