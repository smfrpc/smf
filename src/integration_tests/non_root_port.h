// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#pragma once
namespace smf {
inline uint16_t
non_root_port(uint16_t port) {
  if (port < 1024) return port + 1024;
  return port;
}
}  // namespace smf
