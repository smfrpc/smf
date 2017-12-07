// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <memory>
#include <ostream>
#include <utility>

// seastar
#include <core/distributed.hh>
#include <core/future.hh>
#include <core/sstring.hh>

// smf
#include "filesystem/wal_opts.h"
#include "filesystem/wal_requests.h"
#include "filesystem/wal_topics_manager.h"
#include "platform/macros.h"

namespace smf {

/// brief - write ahead log
class write_ahead_log {
 public:
  explicit write_ahead_log(wal_opts opt);
  /// \brief returns starting offset off a successful write
  seastar::future<wal_write_reply> append(wal_write_request r);
  seastar::future<wal_read_reply> get(wal_read_request r);

  // \brief filesystem monitoring
  seastar::future<> open();
  seastar::future<> close();
  // support seastar shardable
  seastar::future<> stop() { return close(); }

  ~write_ahead_log() = default;
  SMF_DISALLOW_COPY_AND_ASSIGN(write_ahead_log);
  const wal_opts opts;

 private:
  smf::wal_topics_manager tm_;
  smf::wal_page_cache     reader_cache_;
};

}  // namespace smf
