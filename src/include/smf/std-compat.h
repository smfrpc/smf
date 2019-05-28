// Copyright 2018 Alexander Gallego
//
#pragma once

#if __cplusplus >= 201703L && __has_include(<optional>) \
    && __has_include(<string_view>) && __has_include(<variant>)
#define SEASTAR_USE_STD_OPTIONAL_VARIANT_STRINGVIEW
#endif

#include <seastar/util/std-compat.hh>

namespace smf {
namespace compat = seastar::compat;
}  // namespace smf