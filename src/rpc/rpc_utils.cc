#include "rpc/rpc_utils.h"
#include "hashing_utils.h"
namespace smf {
fbs::rpc::Header header_for_payload(const char *payload,
                                    size_t payload_size,
                                    fbs::rpc::Flags flags) {
  using fbs::rpc::Flags;
  auto crc = xxhash_32(payload, payload_size);
  uint32_t header_flags =
    static_cast<uint32_t>(Flags::Flags_CHECKSUM) | static_cast<uint32_t>(flags);
  return fbs::rpc::Header(payload_size, static_cast<Flags>(header_flags), crc);
}
}
