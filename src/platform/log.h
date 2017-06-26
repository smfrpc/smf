// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <fmt/printf.h>
#include <util/log.hh>

#include "platform/macros.h"

namespace smf {
namespace log_detail {
// A small helper for throw_if_null().
template <typename T>
T *throw_if_null(const char *file, int line, const char *names, T *t);
inline void noop(...) { /*silences compiler errors*/
}
}  // namespace log_detail
}  // namespace smf


static seastar::logger _smf_log("smf");
#define SET_LOG_LEVEL(level) _smf_log.set_level(level)
#define __FILENAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG_INFO(format, args...) \
  _smf_log.info("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_ERROR(format, args...) \
  _smf_log.error("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_WARN(format, args...) \
  _smf_log.warn("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_DEBUG(format, args...) \
  _smf_log.debug("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_TRACE(format, args...) \
  _smf_log.trace("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_THROW(format, args...)                                           \
  do {                                                                       \
    auto s = fmt::sprintf("{}:{}] " format, __FILENAME__, __LINE__, ##args); \
    _smf_log.error(s.c_str());                                               \
    throw std::runtime_error(s.c_str());                                     \
  } while (0)

#define THROW_IFNULL(val)                            \
  smf::log_detail::throw_if_null(__FILE__, __LINE__, \
                                 "'" #val "' Must be non NULL", (val))
#define LOG_INFO_IF(condition, format, args...)                                \
  do {                                                                         \
    if (condition) {                                                           \
      _smf_log.info("{}:{}] (" #condition ") " format, __FILENAME__, __LINE__, \
                    ##args);                                                   \
    }                                                                          \
  } while (0)
#define LOG_ERROR_IF(condition, format, args...)                      \
  do {                                                                \
    if (condition) {                                                  \
      _smf_log.error("{}:{}] (" #condition ") " format, __FILENAME__, \
                     __LINE__, ##args);                               \
    }                                                                 \
  } while (0)
#define LOG_DEBUG_IF(condition, format, args...)                      \
  do {                                                                \
    if (condition) {                                                  \
      _smf_log.debug("{}:{}] (" #condition ") " format, __FILENAME__, \
                     __LINE__, ##args);                               \
    }                                                                 \
  } while (0)
#define LOG_WARN_IF(condition, format, args...)                                \
  do {                                                                         \
    if (condition) {                                                           \
      _smf_log.warn("{}:{}] (" #condition ") " format, __FILENAME__, __LINE__, \
                    ##args);                                                   \
    }                                                                          \
  } while (0)
#define LOG_TRACE_IF(condition, format, args...)                      \
  do {                                                                \
    if (condition) {                                                  \
      _smf_log.trace("{}:{}] (" #condition ") " format, __FILENAME__, \
                     __LINE__, ##args);                               \
    }                                                                 \
  } while (0)
#define LOG_THROW_IF(condition, format, args...)                             \
  do {                                                                       \
    if (SMF_UNLIKELY(condition)) {                                           \
      auto s = fmt::sprintf("{}:{}] (" #condition ") " format, __FILENAME__, \
                            __LINE__, ##args);                               \
      _smf_log.error(s.c_str());                                             \
      throw std::runtime_error(s.c_str());                                   \
    }                                                                        \
  } while (0)


#ifndef NDEBUG
#define DTHROW_IFNULL(val)                           \
  smf::log_detail::throw_if_null(__FILE__, __LINE__, \
                                 "'" #val "' Must be non NULL", (val))
#define DLOG_INFO(format, args...) \
  _smf_log.info("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_ERROR(format, args...) \
  _smf_log.error("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_WARN(format, args...) \
  _smf_log.warn("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_DEBUG(format, args...) \
  _smf_log.debug("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_TRACE(format, args...) \
  _smf_log.trace("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_INFO_IF(condition, format, args...)                              \
  do {                                                                        \
    if (condition) {                                                          \
      _smf_log.info("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, \
                    ##args);                                                  \
    }                                                                         \
  } while (0)
#define DLOG_ERROR_IF(condition, format, args...)                              \
  do {                                                                         \
    if (condition) {                                                           \
      _smf_log.error("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, \
                     ##args);                                                  \
    }                                                                          \
  } while (0)
#define DLOG_DEBUG_IF(condition, format, args...)                              \
  do {                                                                         \
    if (condition) {                                                           \
      _smf_log.debug("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, \
                     ##args);                                                  \
    }                                                                          \
  } while (0)

#define DLOG_WARN_IF(condition, format, args...)                              \
  do {                                                                        \
    if (condition) {                                                          \
      _smf_log.warn("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, \
                    ##args);                                                  \
    }                                                                         \
  } while (0)
#define DLOG_TRACE_IF(condition, format, args...)                              \
  do {                                                                         \
    if (condition) {                                                           \
      _smf_log.trace("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, \
                     ##args);                                                  \
    }                                                                          \
  } while (0)
#define DLOG_THROW_IF(condition, format, args...)                            \
  do {                                                                       \
    if (SMF_UNLIKELY(condition)) {                                           \
      auto s = fmt::sprintf("{}:{}] (" #condition ") " format, __FILENAME__, \
                            __LINE__, ##args);                               \
      _smf_log.error(s.c_str());                                             \
      throw std::runtime_error(s.c_str());                                   \
    }                                                                        \
  } while (0)

#else
#define DTHROW_IFNULL(x) true ? x : throw /*make compiler happy*/
#define DLOG_INFO(format, args...) smf::log_detail::noop(format, ##args)
#define DLOG_ERROR(format, args...) smf::log_detail::noop(format, ##args)
#define DLOG_WARN(format, args...) smf::log_detail::noop(format, ##args)
#define DLOG_DEBUG(format, args...) smf::log_detail::noop(format, ##args)
#define DLOG_TRACE(format, args...) smf::log_detail::noop(format, ##args)
#define DLOG_INFO_IF(condition, format, args...) \
  smf::log_detail::noop(condition, format, ##args)
#define DLOG_ERROR_IF(condition, format, args...) \
  smf::log_detail::noop(condition, format, ##args)
#define DLOG_DEBUG_IF(condition, format, args...) \
  smf::log_detail::noop(condition, format, ##args)
#define DLOG_WARN_IF(condition, format, args...) \
  smf::log_detail::noop(condition, format, ##args)
#define DLOG_TRACE_IF(condition, format, args...) \
  smf::log_detail::noop(condition, format, ##args)
#define DLOG_THROW_IF(condition, format, args...) \
  smf::log_detail::noop(condition, format, ##args)
#endif


namespace smf {
namespace log_detail {
// A small helper for CHECK_NOTNULL().
template <typename T>
T *throw_if_null(const char *file, int line, const char *names, T *t) {
  if (SMF_UNLIKELY(t == NULL)) {
    auto s = fmt::sprintf("{}:{}] check_not_null({})", file, line, names);
    _smf_log.error(s.c_str());
    throw std::runtime_error(s.c_str());
  }
  return t;
}
}  // namespace log_detail
}  // namespace smf
