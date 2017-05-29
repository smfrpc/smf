// Copyright 2017 Alexander Gallego
//
#pragma once

#include <flatbuffers/flatbuffers.h>

namespace smf {
// clang does not yet know how to format concepts
// so putting all concepts on a different section
// clang-format off

    template <typename T>
    concept bool FlatBuffersNativeTable = requires(T a) {
      { std::is_base_of<flatbuffers::NativeTable, typename T::NativeTableType>::value == true };
      { std::is_base_of<flatbuffers::Table, T>::value == true };
    }; // NOLINT

// clang-format on
} // namespace smf
