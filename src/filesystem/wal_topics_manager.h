// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <memory>
#include <unordered_map>

#include <core/shared_ptr.hh>

#include "filesystem/wal_opts.h"
#include "filesystem/wal_partition_manager.h"
#include "platform/macros.h"

namespace smf {

class partition_manager_list {
 public:
  partition_manager_list(const wal_opts &o, const seastar::sstring &t);

  seastar::future<seastar::lw_shared_ptr<smf::wal_partition_manager>>
  get_manager(uint32_t partition);
  seastar::future<> close();

  const seastar::sstring topic;
  const wal_opts &       opts;

  SMF_DISALLOW_COPY_AND_ASSIGN(partition_manager_list);

 private:
  std::unordered_map<uint32_t, seastar::lw_shared_ptr<wal_partition_manager>>
                     pmanagers_{};
  seastar::semaphore serialize_open_{1};
};

class wal_topics_manager {
 public:
  explicit wal_topics_manager(const wal_opts &o);

  seastar::future<seastar::lw_shared_ptr<wal_partition_manager>> get_manager(
    const seastar::sstring &topic, uint32_t partition);

  seastar::future<> close();
  const wal_opts &  opts;
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_topics_manager);

 private:
  std::unordered_map<seastar::sstring,
                     seastar::lw_shared_ptr<partition_manager_list>>
                     mngrs_;
  seastar::semaphore serialize_open_{1};
};

}  // namespace smf
