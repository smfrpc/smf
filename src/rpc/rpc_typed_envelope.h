// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#pragma once
// smf
#include "platform/macros.h"

#include "rpc/rpc_envelope.h"

namespace smf {
template <typename T> rpc_typed_envelope {
  using typename type = T;


  rpc_typed_envelope(T && t) : envelope(nullptr) {
  }
  rpc_envelope envelope;
  /// \brief copy ctor deleted
  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_typed_envelope);
};
}  // namespace smf
