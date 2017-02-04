// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <core/reactor.hh>
// smf
#include "filesystem/wal_file_walker.h"

namespace smf {
struct wal_file_name_mender : wal_file_walker {
  wal_file_name_mender(file d) : wal_file_walker(std::move(d)) {}

  future<> visit(directory_entry de) final {
    if (is_name_locked(de)) {
      return rename_file(de.name, name_without_prefix(de));
    }
    return make_ready_future<>();
  }
};

}  // namespace smf
