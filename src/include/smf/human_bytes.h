// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <iomanip>
#include <ostream>
namespace smf {

/// \brief prints a double as bytes, KB, MB, GB, TB, etc.
/// \code `std::cout << human_bytes(x) << std::endl;` \endcode
///
struct human_bytes {
  explicit human_bytes(double d) : data(d) {}
  const double data;
};
inline std::ostream &
operator<<(std::ostream &o, const ::smf::human_bytes &h) {
  auto const orig_precision = o.precision();
  o << std::fixed << std::setprecision(3);
  auto t = h.data;
  if (t < 1024) {
    if (t < 1) { t = 0; }
    o << t << " bytes";
  } else if ((t /= static_cast<double>(1024)) < static_cast<double>(1000)) {
    o << t << " KB";
  } else if ((t /= static_cast<double>(1024)) < static_cast<double>(1000)) {
    o << t << " MB";
  } else if ((t /= static_cast<double>(1024)) < static_cast<double>(1000)) {
    o << t << " GB";
  } else {
    o << t << " TB";
  }
  // reset to original
  o << std::setprecision(orig_precision);
  return o;
}

}  // namespace smf
