#pragma once
#include <boost/crc.hpp>
#include <xxhash.h>
namespace smf {
inline uint32_t crc_32(const char *data, const size_t &length) {
  boost::crc_32_type result;
  result.process_bytes(data, length);
  return result.checksum();
}
inline uint32_t xxhash(const char *data, const size_t &length) {
  constexpr static uint64_t kTrim =
    uint64_t(0) | std::numeric_limits<uint32_t>::max();
  // clean up the top bits after the hash
  uint64_t t = XXH64(data, length, 0) & kTrim;
  return static_cast<uint32_t>(t);
}
}
