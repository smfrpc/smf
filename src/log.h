#pragma once
#include <util/log.hh>
#include <fmt/format.h>

namespace smf {
namespace log_detail {
// A small helper for throw_if_null().
template <typename T>
T *throw_if_null(const char *file, int line, const char *names, T *t);
} // namespace log_detail
} // namespace smf


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
                                 "'" #val "' Must be non NULL", (val));
#ifdef NDEBUG
#define DTHROW_IFNULL(val) THROW_NOTNULL(val)
#else
#define LOG_NOOP ((void)0)
#define DTHROW_IFNULL(val) (val)
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
