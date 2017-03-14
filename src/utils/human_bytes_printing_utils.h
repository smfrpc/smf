// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <iomanip>
#include <ostream>
namespace smf {

template <typename T> static std::ostream &human_bytes(std::ostream &o, T t) {
  auto const orig_precision = o.precision();
  o << std::fixed << std::setprecision(3);
  if (t < 100) {
    if (t < 1) {
      t = 0;
    }
    o << t << " bytes";
  } else if ((t /= T(1024)) < T(1000)) {
    o << t << " KB";
  } else if ((t /= T(1024)) < T(1000)) {
    o << t << " MB";
  } else if ((t /= T(1024)) < T(1000)) {
    o << t << " GB";
  } else {
    o << t << " TB";
  }
  std::setprecision(orig_precision);
  return o;
}


}  // namespace smf
