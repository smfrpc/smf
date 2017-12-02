// Copyright 2017 Alexander Gallego
//
#pragma once

#include <boost/intrusive/set.hpp>
#include <boost/intrusive/unordered_set.hpp>

#include "platform/macros.h"

namespace smf {
namespace clock_pro_internal {
// clang does not yet know how to format concepts
// so putting all concepts on a different section
// clang-format off
SMF_CONCEPT(
template <typename T>
concept bool ChunkValue = std::is_move_constructible<T>::value;

template <typename T>
concept bool ChunkKey = requires(T a, T b) {
  { a == b } -> bool;
  { a != b } -> bool;
  { a < b }  -> bool;
}; // NOLINT
)
// clang-format on

template <typename key_type, typename value_type>
SMF_CONCEPT(requires ChunkKey<key_type> and ChunkValue<value_type>)
struct chunk : public boost::intrusive::set_base_hook<>,
               public boost::intrusive::unordered_set_base_hook<> {
  chunk() = delete;

  chunk(key_type &&k, value_type &&v) : key(std::move(k)), data(std::move(v)) {}

  chunk(key_type &&k, value_type &&v, bool _ref)
    : key(std::move(k)), data(std::move(v)), ref(_ref) {}

  chunk(chunk &&c) noexcept
    : key(std::move(c.key)), data(std::move(c.data)), ref(std::move(c.ref)) {}
  ~chunk() {}
  chunk(const chunk &) = delete;
  void operator=(const chunk &) = delete;

  chunk<key_type, value_type> *self() { return this; }

  key_type   key;
  value_type data;
  bool       ref{false};
};
}  // namespace clock_pro_internal
}  // namespace smf
