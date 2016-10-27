#pragma once
#include <boost/crc.hpp>
#include <xxhash.h>
namespace smf {
inline uint32_t crc_32(const char *data, const size_t &length) {
  boost::crc_32_type result;
  result.process_bytes(data, length);
  return result.checksum();
}
inline uint64_t xxhash_64(const char *data, const size_t &length) {
  return XXH64(data, length, 0);
}
inline uint32_t xxhash_32(const char *data, const size_t &length) {
  return XXH32(data, length, 0);
}
}
