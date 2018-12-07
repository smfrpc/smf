// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <chrono>

#include <seastar/core/lowres_clock.hh>
#include <seastar/core/sstring.hh>

namespace smf {
inline uint64_t
time_now_millis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::system_clock::now().time_since_epoch())
    .count();
}

inline uint64_t
time_now_micros() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
           std::chrono::high_resolution_clock::now().time_since_epoch())
    .count();
}

inline uint64_t
lowres_time_now_millis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
           seastar::lowres_clock::now().time_since_epoch())
    .count();
}
// Extracted out of gmock
// Converts the given epoch time in milliseconds to a date string in the ISO
// 8601 format, without the timezone information.
inline seastar::sstring
time_as_iso_8601(uint64_t millisecs) {
  // Using non-reentrant version as localtime_r is not portable.
  time_t seconds = static_cast<time_t>(millisecs / 1000);
  const struct tm *const time_struct = localtime(&seconds);  // NOLINT
  if (time_struct == NULL) {
    return "";  // Invalid ms value
  }
  // YYYY-MM-DDThh:mm:ss
  return seastar::to_sstring(time_struct->tm_year + 1900) + "-" +
         seastar::to_sstring(time_struct->tm_mon + 1) + "-" +
         seastar::to_sstring(time_struct->tm_mday) + "T" +
         seastar::to_sstring(time_struct->tm_hour) + ":" +
         seastar::to_sstring(time_struct->tm_min) + ":" +
         seastar::to_sstring(time_struct->tm_sec);
}
}  // namespace smf
