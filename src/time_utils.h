// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <chrono>
// third party
#include <core/sstring.hh>

namespace smf {
inline uint64_t millis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::system_clock::now().time_since_epoch())
    .count();
}
// Extracted out of gmock
// Converts the given epoch time in milliseconds to a date string in the ISO
// 8601 format, without the timezone information.
inline sstring timeInMillisAsIso8601(uint64_t ms) {
  // Using non-reentrant version as localtime_r is not portable.
  time_t                 seconds     = static_cast<time_t>(ms / 1000);
  const struct tm *const time_struct = localtime(&seconds);  // NOLINT
  if (time_struct == NULL) {
    return "";  // Invalid ms value
  }
  // YYYY-MM-DDThh:mm:ss
  return to_sstring(time_struct->tm_year + 1900) + "-"
         + to_sstring(time_struct->tm_mon + 1) + "-"
         + to_sstring(time_struct->tm_mday) + "T"
         + to_sstring(time_struct->tm_hour) + ":"
         + to_sstring(time_struct->tm_min) + ":"
         + to_sstring(time_struct->tm_sec);
}
}  // namespace smf
