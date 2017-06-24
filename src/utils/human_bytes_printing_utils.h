// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <ostream>
namespace smf {

struct human_bytes {
  /// \brief prints a double as bytes, KB, MB, GB, TB, etc.
  ///
  static std::ostream &print(std::ostream &o, double t);
};

}  // namespace smf
