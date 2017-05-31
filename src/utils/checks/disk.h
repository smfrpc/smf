// Copyright 2017 Alexander Gallego
//
#pragma once

#include <core/future.hh>
#include <core/sstring.hh>

namespace smf {
namespace checks {
struct disk {
  static seastar::future<> check(seastar::sstring path, bool ignore = false);
};
}  // namespace checks
}  // namespace smf
