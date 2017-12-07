// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <cstdint>

#include <core/align.hh>

namespace smf {
/// \brief given a page alighment and an offset within the bounds of a page
/// returns the bottom offset of the page
inline uint64_t
offset_to_page(uint64_t offset, uint64_t page_alignment) {
  return seastar::align_down(offset, page_alignment) / page_alignment;
}
}  // namespace smf
