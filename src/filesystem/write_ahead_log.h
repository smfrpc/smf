// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <memory>
#include <ostream>
#include <utility>

// seastar
#include <core/future.hh>
#include <core/sstring.hh>

// smf
#include "filesystem/wal_opts.h"
#include "filesystem/wal_requests.h"

namespace smf {
class write_ahead_log;

static std::unique_ptr<write_ahead_log> make_unique_wal(wal_opts opts);


/// brief - write ahead log
class write_ahead_log {
 public:
  explicit write_ahead_log(wal_opts o) : opts(std::move(o)) {}
  /// \brief returns starting offset off a successful write
  virtual seastar::future<wal_write_reply> append(wal_write_request r) = 0;
  virtual seastar::future<> invalidate(wal_write_invalidation r)       = 0;
  virtual seastar::future<wal_read_reply> get(wal_read_request r)      = 0;

  // \brief filesystem monitoring
  virtual seastar::future<> open()  = 0;
  virtual seastar::future<> close() = 0;
  virtual ~write_ahead_log() {}

  // support seastar shardable
  seastar::future<> stop() final { return close(); }

  const wal_opts opts;
};

}  // namespace smf
