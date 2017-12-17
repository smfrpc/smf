// Copyright 2017 Alexander Gallego
//
#include "utils/checks/disk.h"

#include <core/reactor.hh>

#include "platform/log.h"

namespace smf {
namespace checks {

seastar::future<> disk::check(seastar::sstring path) {
  return check_direct_io_support(path).then([path] {
    return file_system_at(path).then([path](auto fs) {
      if (fs != seastar::fs_type::xfs) {
        LOG_ERROR("Path: `{}' is not on XFS. This is a non-supported setup. "
                  "Expect poor performance.",
                  path);
      }
    });
  });
}

}  // namespace checks
}  // namespace smf
