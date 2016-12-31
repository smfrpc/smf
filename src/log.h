#pragma once
#include <fmt/format.h>
#include <util/log.hh>

namespace smf {
namespace log_detail {
// A small helper for throw_if_null().
template <typename T>
T *throw_if_null(const char *file, int line, const char *names, T *t);
inline void noop(...) { /*silences compiler errors*/
}
} // namespace log_detail
} // namespace smf


// TODO(agallego) - clean this up. ugh. use LOG(LEVEL,..) or smth
namespace smf {
static seastar::logger log("smf");
#define __FILENAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG_INFO(format, args...) \
  log.info("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_ERROR(format, args...) \
  log.error("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_WARN(format, args...) \
  log.warn("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_DEBUG(format, args...) \
  log.debug("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_TRACE(format, args...) \
  log.trace("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_THROW(format, args...)                                           \
  do {                                                                       \
    auto s = fmt::sprintf("{}:{}] " format, __FILENAME__, __LINE__, ##args); \
    log.error(s.c_str());                                                    \
    throw std::runtime_error(s.c_str());                                     \
  } while(0)

#define THROW_IFNULL(val)                            \
  smf::log_detail::throw_if_null(__FILE__, __LINE__, \
                                 "'" #val "' Must be non NULL", (val))

#define LOG_INFO_IF(condition, format, args...) \
  log.info("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, ##args)
#define LOG_ERROR_IF(condition, format, args...) \
  log.error("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, ##args)
#define LOG_DEBUG_IF(condition, format, args...) \
  log.debug("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, ##args)
#define LOG_WARN_IF(condition, format, args...) \
  log.warn("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, ##args)
#define LOG_TRACE_IF(condition, format, args...) \
  log.trace("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, ##args)
#define LOG_THROW_IF(condition, format, args...)                  \
  do {                                                            \
    auto s = fmt::sprintf("{}:{}] FAIL: '" #condition "'" format, \
                          __FILENAME__, __LINE__, ##args);        \
    log.error(s.c_str());                                         \
    throw std::runtime_error(s.c_str());                          \
  } while(condition)


#ifndef NDEBUG
#define DTHROW_IFNULL(val)                           \
  smf::log_detail::throw_if_null(__FILE__, __LINE__, \
                                 "'" #val "' Must be non NULL", (val))
#define DLOG_INFO(format, args...) \
  log.info("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_ERROR(format, args...) \
  log.error("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_WARN(format, args...) \
  log.warn("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_DEBUG(format, args...) \
  log.debug("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_TRACE(format, args...) \
  log.trace("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_INFO_IF(condition, format, args...) \
  log.info("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, ##args)
#define DLOG_ERROR_IF(condition, format, args...) \
  log.error("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, ##args)
#define DLOG_DEBUG_IF(condition, format, args...) \
  log.debug("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, ##args)
#define DLOG_WARN_IF(condition, format, args...) \
  log.warn("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, ##args)
#define DLOG_TRACE_IF(condition, format, args...) \
  log.trace("{}:{}] (" #condition ")" format, __FILENAME__, __LINE__, ##args)
#define DLOG_THROW_IF(condition, format, args...)                 \
  do {                                                            \
    auto s = fmt::sprintf("{}:{}] FAIL: '" #condition "'" format, \
                          __FILENAME__, __LINE__, ##args);        \
    log.error(s.c_str());                                         \
    throw std::runtime_error(s.c_str());                          \
  } while(condition)

#else
#define DTHROW_IFNULL(x) true ? x : throw /*make compiler happy*/
#define DLOG_INFO(format, args...) log_detail::noop(format, ##args)
#define DLOG_ERROR(format, args...) log_detail::noop(format, ##args)
#define DLOG_WARN(format, args...) log_detail::noop(format, ##args)
#define DLOG_DEBUG(format, args...) log_detail::noop(format, ##args)
#define DLOG_TRACE(format, args...) log_detail::noop(format, ##args)
#define DLOG_INFO_IF(condition, format, args...) \
  log_detail::noop(condition, format, ##args)
#define DLOG_ERROR_IF(condition, format, args...) \
  log_detail::noop(condition, format, ##args)
#define DLOG_DEBUG_IF(condition, format, args...) \
  log_detail::noop(condition, format, ##args)
#define DLOG_WARN_IF(condition, format, args...) \
  log_detail::noop(condition, format, ##args)
#define DLOG_TRACE_IF(condition, format, args...) \
  log_detail::noop(condition, format, ##args)
#define DLOG_THROW_IF(condition, format, args...) \
  log_detail::noop(condition, format, ##args)
#endif

} // namespace smf

namespace smf {
namespace log_detail {
// A small helper for CHECK_NOTNULL().
template <typename T>
T *throw_if_null(const char *file, int line, const char *names, T *t) {
  if(t == NULL) {
    auto s = fmt::sprintf("{}:{}] check_not_null({})", file, line, names);
    log.error(s.c_str());
    throw std::runtime_error(s.c_str());
  }
  return t;
}
} // namespace log_detail
} // namespace smf
