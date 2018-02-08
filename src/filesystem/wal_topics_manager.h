// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <memory>

#include <core/shared_ptr.hh>

#include "adt/flat_hash_map.h"
#include "filesystem/wal_opts.h"
#include "filesystem/wal_partition_manager.h"
#include "filesystem/wal_ptid.h"
#include "platform/macros.h"

namespace smf {
class wal_topics_manager {
 public:
  explicit wal_topics_manager(wal_opts o);

  seastar::future<wal_partition_manager *> get_manager(seastar::sstring topic,
                                                       uint32_t partition);

  seastar::future<> close();
  wal_opts          opts;
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_topics_manager);

 private:
  smf::flat_hash_map<wal_ptid, std::unique_ptr<wal_partition_manager>> mngrs_;
  seastar::semaphore serialize_open_{1};
};

}  // namespace smf
