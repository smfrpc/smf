// Copyright 2018 SMF Authors
//

#pragma once
#include <boost/crc.hpp>
namespace smf_gen {
inline uint32_t
crc32(const char *data, const size_t &length) {
  boost::crc_32_type result;
  result.process_bytes(data, length);
  return result.checksum();
}
}  // namespace smf_gen
