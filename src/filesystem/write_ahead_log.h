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

namespace smf {
class write_ahead_log;
class write_ahead_log_proxy;

/// \brief main interface while using seastar.
///
using sharded_write_ahead_log = seastar::distributed<write_ahead_log_proxy>;

std::unique_ptr<write_ahead_log> make_unique_wal(wal_opts opts);

/// brief - write ahead log
class write_ahead_log {
 public:
  write_ahead_log() {}
  /// \brief returns starting offset off a successful write
  virtual seastar::future<wal_write_reply> append(wal_write_request r) = 0;
  virtual seastar::future<wal_read_reply> get(wal_read_request r)      = 0;

  // \brief filesystem monitoring
  virtual seastar::future<> open()  = 0;
  virtual seastar::future<> close() = 0;
  virtual ~write_ahead_log() {}

  // support seastar shardable
  virtual seastar::future<> stop() final { return close(); }
};

/// \brief needed for seastar::distributed concept
///
class write_ahead_log_proxy : public write_ahead_log {
 public:
  explicit write_ahead_log_proxy(wal_opts  o) : wal_(make_unique_wal(o)) {}
  virtual seastar::future<wal_write_reply> append(wal_write_request r) final {
    return wal_->append(std::move(r));
  }
  virtual seastar::future<wal_read_reply> get(wal_read_request r) final {
    return wal_->get(std::move(r));
  }
  virtual seastar::future<> open() final { return wal_->open(); }
  virtual seastar::future<> close() final { return wal_->close(); }
  virtual ~write_ahead_log_proxy() {}

 private:
  std::unique_ptr<write_ahead_log> wal_;
};


}  // namespace smf
