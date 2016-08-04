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
  uint64_t t = XXH64(data, length, 0);
  return static_cast<uint32_t>(t);
}
}
