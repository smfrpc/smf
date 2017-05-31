// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#pragma once

#include "rpc/rpc_letter_concepts.h"

#include "platform/macros.h"
#include "rpc/rpc_envelope.h"

namespace smf {
template <typename RootType>
requires FlatBuffersNativeTable<RootType> struct rpc_typed_envelope {
  using type = typename RootType::NativeTableType;

  rpc_envelope          envelope;
  std::unique_ptr<type> data;


  rpc_typed_envelope() : data(std::make_unique<type>()) {}
  ~rpc_typed_envelope() {}
  rpc_typed_envelope(std::unique_ptr<type> &&ptr) noexcept
    : data(std::move(ptr)) {}
  rpc_typed_envelope &operator=(rpc_typed_envelope<RootType> &&te) noexcept {
    envelope = std::move(te.envelope);
    data     = std::move(te.data);
    return *this;
  }
  rpc_typed_envelope(rpc_typed_envelope<RootType> &&te) noexcept {
    *this = std::move(te);
  }
  /// \brief this copies this->data into this->envelope
  /// and retuns a *moved* copy of the envelope. That is
  /// the envelope will be invalid after this method call
  rpc_envelope &&serialize_data() {
    rpc_letter::serialize_type_into_letter<RootType>(*(data.get()),
                                                     &envelope.letter);
    return std::move(envelope);
  }
  /// \brief copy ctor deleted
  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_typed_envelope);
};
}  // namespace smf
