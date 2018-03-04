// Copyright 2017 Alexander Gallego
//
#pragma once

#include <flatbuffers/flatbuffers.h>

#include "smf/macros.h"

namespace smf {
// clang does not yet know how to format concepts
// so putting all concepts on a different section
// clang-format off
SMF_CONCEPT(
template <typename T>
concept bool FlatBuffersNativeTable = requires(T a) {
  { std::is_base_of<flatbuffers::NativeTable,                // NOLINT
      typename T::NativeTableType>::value == true };         // NOLINT
  { std::is_base_of<flatbuffers::Table, T>::value == true }; // NOLINT
};                                                           // NOLINT
)
// clang-format on
}  // namespace smf
