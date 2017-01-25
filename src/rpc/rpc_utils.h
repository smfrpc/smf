// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include "rpc_generated.h"
namespace smf {
fbs::rpc::Header header_for_payload(
  const char *    payload,
  size_t          payload_size,
  fbs::rpc::Flags flags = fbs::rpc::Flags::Flags_NONE);


inline uint32_t ftoi(fbs::rpc::Flags f) { return static_cast<uint32_t>(f); }
inline fbs::rpc::Flags itof(uint32_t i) {
  return static_cast<fbs::rpc::Flags>(i);
}

}  // namespace smf
