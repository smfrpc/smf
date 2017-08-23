#pragma once

#include "hashing/hashing_utils.h"
#include "rpc_generated.h"

namespace smf {
inline void checksum_rpc_payload(rpc::header &hdr,
                                 const char * payload,
                                 uint32_t     size) {
  hdr.mutate_checksum(xxhash_32(payload, size));
  // we do not want to cast it to the underlying type. flatbuffers structs -
  // destination uses integers to store enums, but the definition varies in size
  // for the codgen it produces :'( - which means we need to cast it to the
  // destination container, not the enum::underlying_type<T>::value
  //
  using v_t = decltype(hdr.validation());
  static constexpr auto checksum =
    rpc::validation_flags::validation_flags_checksum;
  auto v = static_cast<v_t>(hdr.validation()) | static_cast<v_t>(checksum);
  hdr.mutate_validation(static_cast<rpc::validation_flags>(v));
  hdr.mutate_size(size);
}
}
