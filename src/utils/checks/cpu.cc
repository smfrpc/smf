// Copyright 2017 Alexander Gallego
//
#include "utils/checks/cpu.h"

#include "platform/log.h"

namespace smf {
namespace checks {
void
cpu::check() {
  LOG_THROW_IF(!__builtin_cpu_supports("sse4.2"),
               "sse4.2 support is required to run");
}
}  // namespace checks
}  // namespace smf
