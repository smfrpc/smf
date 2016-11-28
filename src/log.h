#pragma once
#include <util/log.hh>

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

} // namespace smf
