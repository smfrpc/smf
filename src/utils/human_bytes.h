// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <ostream>
namespace smf {


/// \brief prints a double as bytes, KB, MB, GB, TB, etc.
/// \code `std::cout << human_bytes(x) << std::endl;` \endcode
///
struct human_bytes {
  explicit human_bytes(double d) : data(d) {}
  const double data;
};
std::ostream &operator<<(std::ostream &o, const human_bytes &h);

}  // namespace smf
