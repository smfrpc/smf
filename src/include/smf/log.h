// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#ifndef SMF_PLATFORM_LOG_H
#define SMF_PLATFORM_LOG_H

#include <fmt/printf.h>
#include <util/log.hh>

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
// A small helper for throw_if_null().
template <typename T>
T *
throw_if_null(const char *file, int line, const char *names, T *t) {
  if (SMF_UNLIKELY(t == NULL)) {
    // TODO(agallego) - remove fmt from public headers
    //
    auto s = fmt::sprintf("{}:{}] check_not_null({})", file, line, names);
    smf::internal_logger::get().error(s.c_str());
    throw std::runtime_error(s.c_str());
  }
  return t;
}
inline void
noop(...) {}

}  // namespace log_detail
}  // namespace smf

#define SET_LOG_LEVEL(level) smf::internal_logger::get().set_level(level)
#define __FILENAME__                                                           \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
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
  smf::internal_logger::get().trace("{}:{}] " format, __FILENAME__, __LINE__,  \
                                    ##args)
#define LOG_THROW(format, args...)                                             \
  do {                                                                         \
    fmt::MemoryWriter __smflog_w;                                              \
    __smflog_w.write("{}:{}] " format, __FILENAME__, __LINE__, ##args);        \
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
      fmt::MemoryWriter __smflog_w;                                            \
      __smflog_w.write("{}:{}] (" #condition ") " format, __FILENAME__,        \
                       __LINE__, ##args);                                      \
      smf::internal_logger::get().error(__smflog_w.data());                    \
      throw std::runtime_error(__smflog_w.data());                             \
    }                                                                          \
  } while (false)

#ifndef NDEBUG
#define DTHROW_IFNULL(val)                                                     \
  smf::log_detail::throw_if_null(__FILE__, __LINE__,                           \
                                 "'" #val "' Must be non NULL", (val))
#define DLOG_INFO(format, args...)                                             \
  smf::internal_logger::get().info("{}:{}] " format, __FILENAME__, __LINE__,   \
                                   ##args)
#define DLOG_ERROR(format, args...)                                            \
  smf::internal_logger::get().error("{}:{}] " format, __FILENAME__, __LINE__,  \
                                    ##args)
#define DLOG_WARN(format, args...)                                             \
  smf::internal_logger::get().warn("{}:{}] " format, __FILENAME__, __LINE__,   \
                                   ##args)
#define DLOG_DEBUG(format, args...)                                            \
  smf::internal_logger::get().debug("{}:{}] " format, __FILENAME__, __LINE__,  \
                                    ##args)
#define DLOG_TRACE(format, args...)                                            \
  smf::internal_logger::get().trace("{}:{}] " format, __FILENAME__, __LINE__,  \
                                    ##args)
#define DLOG_INFO_IF(condition, format, args...)                               \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().info("{}:{}] (" #condition ") " format,      \
                                       __FILENAME__, __LINE__, ##args);        \
    }                                                                          \
  } while (false)
#define DLOG_ERROR_IF(condition, format, args...)                              \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().error("{}:{}] (" #condition ") " format,     \
                                        __FILENAME__, __LINE__, ##args);       \
    }                                                                          \
  } while (false)
#define DLOG_DEBUG_IF(condition, format, args...)                              \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().debug("{}:{}] (" #condition ") " format,     \
                                        __FILENAME__, __LINE__, ##args);       \
    }                                                                          \
  } while (false)

#define DLOG_WARN_IF(condition, format, args...)                               \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().warn("{}:{}] (" #condition ") " format,      \
                                       __FILENAME__, __LINE__, ##args);        \
    }                                                                          \
  } while (false)
#define DLOG_TRACE_IF(condition, format, args...)                              \
  do {                                                                         \
    if (condition) {                                                           \
      smf::internal_logger::get().trace("{}:{}] (" #condition ") " format,     \
                                        __FILENAME__, __LINE__, ##args);       \
    }                                                                          \
  } while (false)
#define DLOG_THROW_IF(condition, format, args...)                              \
  do {                                                                         \
    if (SMF_UNLIKELY(condition)) {                                             \
      fmt::MemoryWriter __smflog_w;                                            \
      __smflog_w.write("{}:{}] (" #condition ") " format, __FILENAME__,        \
                       __LINE__, ##args);                                      \
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
#define DLOG_INFO_IF(condition, format, args...) ((void)0)
#define DLOG_ERROR_IF(condition, format, args...) ((void)0)
#define DLOG_DEBUG_IF(condition, format, args...) ((void)0)
#define DLOG_WARN_IF(condition, format, args...) ((void)0)
#define DLOG_TRACE_IF(condition, format, args...) ((void)0)
#define DLOG_THROW_IF(condition, format, args...)                              \
  smf::log_detail::noop(condition, format, ##args);
#endif

#endif  // SMF_PLATFORM_LOG_H
