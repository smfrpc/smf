// Copyright 2017 Alexander Gallego
//
#include "utils/checks/memory.h"

#include <sstream>

#include <core/reactor.hh>

#include "platform/log.h"
#include "utils/human_bytes_printing_utils.h"

namespace smf {
namespace checks {

void memory::check(bool ignore) {
  static const uint64_t kMinMemory = 1 << 30;
  const auto            shard_mem  = ::memory::stats().total_memory();
  if (shard_mem >= kMinMemory) {
    return;
  }

  std::stringstream ss;
  ss << "Memory below recommended: `";
  smf::human_bytes(ss, kMinMemory) << "'. Actual is: `";
  smf::human_bytes(ss, shard_mem) << "'";
  LOG_THROW_IF(!ignore, "{}", ss.str());
  LOG_ERROR_IF(ignore, "{}", ss.str());
}

}  // namespace smf
}  // namespace checks
