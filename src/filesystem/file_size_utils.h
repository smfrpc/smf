#pragma once

#include <cstdint>

namespace smf {

inline uint64_t offset_to_page(uint64_t offset, uint64_t page_alignment) {
  auto const front = offset & (page_alignment - 1);
  auto const page_offset = offset - front;
  return page_offset / page_alignment;
}

} // namespace smf
