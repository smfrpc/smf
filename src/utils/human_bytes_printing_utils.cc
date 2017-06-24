// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//

#include "utils/human_bytes_printing_utils.h"

#include <iomanip>

namespace smf {
std::ostream &human_bytes::print(std::ostream &o, double t) {
  auto const orig_precision = o.precision();
  o << std::fixed << std::setprecision(3);
  if (t < 1024) {
    if (t < 1) {
      t = 0;
    }
    o << t << " bytes";
  } else if ((t /= double(1024)) < double(1000)) {
    o << t << " KB";
  } else if ((t /= double(1024)) < double(1000)) {
    o << t << " MB";
  } else if ((t /= double(1024)) < double(1000)) {
    o << t << " GB";
  } else {
    o << t << " TB";
  }
  std::setprecision(orig_precision);
  return o;
}

}  // namespace smf
