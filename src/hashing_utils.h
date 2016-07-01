#pragma once
#include <boost/crc.hpp>
namespace smf {
inline uint32_t crc_32(const char *data, const size_t &length) {
  boost::crc_32_type result;
  result.process_bytes(data, length);
  return result.checksum();
}
}
