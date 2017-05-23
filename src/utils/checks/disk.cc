// Copyright 2017 Alexander Gallego
//
#include "utils/checks/disk.h"

#include <core/reactor.hh>

#include "platform/log.h"

namespace smf {
namespace checks {

future<> disk::check(sstring path, bool ignore) {
  return check_direct_io_support(path).then([path, ignore] {
    return file_system_at(path).then([path, ignore](auto fs) {
      if (fs != fs_type::xfs) {
        LOG_THROW_IF(!ignore,
                     "filesystem != fs_type::xfs. Path: `{}' is unsuported.",
                     path);
        LOG_ERROR("Path: `{}' is not on XFS. This is a non-supported setup.",
                  path);
      }
    });
  });
}

}  // namespace checks
}  // namespace smf
