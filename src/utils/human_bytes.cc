// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//

#include "utils/human_bytes.h"

#include <iomanip>

namespace smf {
std::ostream &
operator<<(std::ostream &o, const human_bytes &h) {
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
  std::setprecision(orig_precision);
  return o;
}

}  // namespace smf
