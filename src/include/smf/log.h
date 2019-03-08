// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#ifndef SMF_PLATFORM_LOG_H
#define SMF_PLATFORM_LOG_H

#include <fmt/printf.h>
#include <seastar/util/log.hh>

#include "smf/macros.h"

namespace smf {
struct internal_logger {
  static seastar::logger &
  get() {
    static seastar::logger l("smf");
    return l;
  }
};

namespace log_detail {
/// \brief compile time log helper to print log file name.
/// @sz must be inclusive
static constexpr const char *
find_last_slash(const char *file, std::size_t sz, char x) {
  return sz == 0
           ? file
           : file[sz] == x ? &file[sz + 1] : find_last_slash(file, sz - 1, x);
}

// A small helper for throw_if_null().
template <typename T>
T *
throw_if_null(const char *file, int line, const char *names, T *t) {
  if (SMF_UNLIKELY(t == NULL)) {
    auto s = fmt::sprintf("{}:{}] check_not_null({})", file, line, names);
    smf::internal_logger::get().error(s.c_str());
    throw std::runtime_error(s.c_str());
  }
  return t;
}

template <typename... Args>
inline void
noop(Args &&... args) {
  ((void)0);
}
}  // namespace log_detail

/// brief Reliably takes effect inside a seastar app_
///  seastar::app_template::run(argc, argv, []() {
///    smf::app_run_log_level(seastar::log_level::trace);
///  });
inline static void
app_run_log_level(seastar::log_level l) {
  smf::internal_logger::get().set_level(l);
  seastar::global_logger_registry().set_logger_level("smf", l);
}
}  // namespace smf

#define __FILENAME__                                                           \
  smf::log_detail::find_last_slash(__FILE__, SMF_ARRAYSIZE(__FILE__) - 1, '/')
#define LOG_INFO(format, args...)                                              \
  smf::internal_logger::get().info("{}:{}] " format, __FILENAME__, __LINE__,   \
                                   ##args)
#define LOG_ERROR(format, args...)                                             \
  smf::internal_logger::get().error("{}:{}] " format, __FILENAME__, __LINE__,  \
                                    ##args)
#define LOG_WARN(format, args...)                                              \
  smf::internal_logger::get().warn("{}:{}] " format, __FILENAME__, __LINE__,   \
                                   ##args)
#define LOG_DEBUG(format, args...)                                             \
  smf::internal_logger::get().debug("{}:{}] " format, __FILENAME__, __LINE__,  \
                                    ##args)
#define LOG_TRACE(format, args...)                                             \
  do {                                                                         \
    if (SMF_UNLIKELY(smf::internal_logger::get().is_enabled(                   \
          seastar::log_level::trace))) {                                       \
      smf::internal_logger::get().trace("{}:{}] " format, __FILENAME__,        \
                                        __LINE__, ##args);                     \
    }                                                                          \
  } while (false)

#define LOG_THROW(format, args...)                                             \
  do {                                                                         \
    fmt::memory_buffer __smflog_w;                                             \
    fmt::format_to(__smflog_w, "{}:{}] " format, __FILENAME__, __LINE__,       \
                   ##args);                                                    \
    smf::internal_logger::get().error(__smflog_w.data());                      \
    throw std::runtime_error(__smflog_w.data());                               \
  } while (false)

#define THROW_IFNULL(val)                                                      \
  smf::log_detail::throw_if_null(__FILE__, __LINE__,                           \
                                 "'" #val "' Must be non NULL", (val))
#define LOG_INFO_IF(condition, format, args...)                                \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().info("{}:{}] (" #condition ") " format,      \
                                       __FILENAME__, __LINE__, ##args);        \
    }                                                                          \
  } while (false)
#define LOG_ERROR_IF(condition, format, args...)                               \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().error("{}:{}] (" #condition ") " format,     \
                                        __FILENAME__, __LINE__, ##args);       \
    }                                                                          \
  } while (false)
#define LOG_DEBUG_IF(condition, format, args...)                               \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().debug("{}:{}] (" #condition ") " format,     \
                                        __FILENAME__, __LINE__, ##args);       \
    }                                                                          \
  } while (false)
#define LOG_WARN_IF(condition, format, args...)                                \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().warn("{}:{}] (" #condition ") " format,      \
                                       __FILENAME__, __LINE__, ##args);        \
    }                                                                          \
  } while (false)
#define LOG_TRACE_IF(condition, format, args...)                               \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().trace("{}:{}] (" #condition ") " format,     \
                                        __FILENAME__, __LINE__, ##args);       \
    }                                                                          \
  } while (false)
#define LOG_THROW_IF(condition, format, args...)                               \
  do {                                                                         \
    if (SMF_UNLIKELY(condition)) {                                             \
      fmt::memory_buffer __smflog_w;                                           \
      fmt::format_to(__smflog_w, "{}:{}] (" #condition ") " format,            \
                     __FILENAME__, __LINE__, ##args);                          \
      smf::internal_logger::get().error(__smflog_w.data());                    \
      throw std::runtime_error(__smflog_w.data());                             \
    }                                                                          \
  } while (false)

#ifndef NDEBUG

#define DTHROW_IFNULL(val)                                                     \
  smf::log_detail::throw_if_null(__FILE__, __LINE__,                           \
                                 "D '" #val "' Must be non NULL", (val))
#define DLOG_THROW(format, args...)                                            \
  do {                                                                         \
    fmt::memory_buffer __smflog_w;                                             \
    fmt::format_to(__smflog_w, "D {}:{}] " format, __FILENAME__, __LINE__,     \
                   ##args);                                                    \
    smf::internal_logger::get().error(__smflog_w.data());                      \
    throw std::runtime_error(__smflog_w.data());                               \
  } while (false)

#define DLOG_INFO(format, args...)                                             \
  smf::internal_logger::get().info("D {}:{}] " format, __FILENAME__, __LINE__, \
                                   ##args)
#define DLOG_ERROR(format, args...)                                            \
  smf::internal_logger::get().error("D {}:{}] " format, __FILENAME__,          \
                                    __LINE__, ##args)
#define DLOG_WARN(format, args...)                                             \
  smf::internal_logger::get().warn("D {}:{}] " format, __FILENAME__, __LINE__, \
                                   ##args)
#define DLOG_DEBUG(format, args...)                                            \
  smf::internal_logger::get().debug("D {}:{}] " format, __FILENAME__,          \
                                    __LINE__, ##args)
#define DLOG_TRACE(format, args...)                                            \
  do {                                                                         \
    if (SMF_UNLIKELY(smf::internal_logger::get().is_enabled(                   \
          seastar::log_level::trace))) {                                       \
      smf::internal_logger::get().trace("D {}:{}] " format, __FILENAME__,      \
                                        __LINE__, ##args);                     \
    }                                                                          \
  } while (false)

#define DLOG_INFO_IF(condition, format, args...)                               \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().info("D {}:{}] (" #condition ") " format,    \
                                       __FILENAME__, __LINE__, ##args);        \
    }                                                                          \
  } while (false)
#define DLOG_ERROR_IF(condition, format, args...)                              \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().error("D {}:{}] (" #condition ") " format,   \
                                        __FILENAME__, __LINE__, ##args);       \
    }                                                                          \
  } while (false)
#define DLOG_DEBUG_IF(condition, format, args...)                              \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().debug("D {}:{}] (" #condition ") " format,   \
                                        __FILENAME__, __LINE__, ##args);       \
    }                                                                          \
  } while (false)

#define DLOG_WARN_IF(condition, format, args...)                               \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().warn("D {}:{}] (" #condition ") " format,    \
                                       __FILENAME__, __LINE__, ##args);        \
    }                                                                          \
  } while (false)
#define DLOG_TRACE_IF(condition, format, args...)                              \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().trace("D {}:{}] (" #condition ") " format,   \
                                        __FILENAME__, __LINE__, ##args);       \
    }                                                                          \
  } while (false)
#define DLOG_THROW_IF(condition, format, args...)                              \
  do {                                                                         \
    if (SMF_UNLIKELY(condition)) {                                             \
      fmt::memory_buffer __smflog_w;                                           \
      fmt::format_to(__smflog_w, "D {}:{}] (" #condition ") " format,          \
                     __FILENAME__, __LINE__, ##args);                          \
      smf::internal_logger::get().error(__smflog_w.data());                    \
      throw std::runtime_error(__smflog_w.data());                             \
    }                                                                          \
  } while (false)

#else
#define DTHROW_IFNULL(x) (x)
#define DLOG_INFO(format, args...) ((void)0)
#define DLOG_ERROR(format, args...) ((void)0)
#define DLOG_WARN(format, args...) ((void)0)
#define DLOG_DEBUG(format, args...) ((void)0)
#define DLOG_TRACE(format, args...) ((void)0)
#define DLOG_THROW(format, args...) ((void)0)
#define DLOG_INFO_IF(condition, format, args...) ((void)0)
#define DLOG_ERROR_IF(condition, format, args...) ((void)0)
#define DLOG_DEBUG_IF(condition, format, args...) ((void)0)
#define DLOG_WARN_IF(condition, format, args...) ((void)0)
#define DLOG_TRACE_IF(condition, format, args...) ((void)0)
#define DLOG_THROW_IF(condition, format, args...)                              \
  smf::log_detail::noop(condition, format, ##args);
#endif

#endif  // SMF_PLATFORM_LOG_H
