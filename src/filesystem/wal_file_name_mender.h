// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <core/reactor.hh>
// smf
#include "filesystem/wal_file_walker.h"
#include "filesystem/wal_name_extractor_utils.h"
#include "hashing_utils.h"

namespace smf {
struct wal_file_name_mender : wal_file_walker {
  wal_file_name_mender(file d) : wal_file_walker(std::move(d)) {}

  future<> visit(directory_entry de) final {
    if (wal_name_extractor_utils::is_name_locked(de.name)) {
      // consistent hash
      auto original_core = wal_name_extractor_utils::extract_core(de.name);
      auto moving_core   = jump_consistent_hash(original_core, smp::count);
      // only move iff* it matches our core. Every core is doing the same right
      // now. Core dumps otherwise.
      if (engine().cpu_id() == moving_core) {
        auto name = wal_name_extractor_utils::name_without_prefix(de.name);
        return rename_file(de.name, name);
      }
    }
    return make_ready_future<>();
  }
};

}  // namespace smf
