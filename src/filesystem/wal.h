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
class wal_iface {
 public:
  /// \brief returns starting offset off a successful write
  virtual seastar::future<wal_write_reply> append(wal_write_request r) = 0;
  virtual seastar::future<> invalidate(wal_write_invalidation r)       = 0;
  virtual seastar::future<wal_read_reply> get(wal_read_request r)      = 0;

  // \brief filesystem monitoring
  virtual seastar::future<> open()  = 0;
  virtual seastar::future<> close() = 0;
  virtual ~wal_iface() {}
};

/// brief - write ahead log
class wal : public wal_iface {
 public:
  explicit wal(wal_opts o) : opts(std::move(o)) {}
  virtual ~wal() {}
  /// brief - used by seastar map-reduce
  inline const wal_opts &     get_otps() const { return opts; }
  static std::unique_ptr<wal> make_wal(wal_type type, wal_opts opts);

 protected:
  wal_opts opts;
};

// simple wrapper for seastar constructs
class shardable_wal : public wal_iface {
 public:
  shardable_wal(wal_type type, wal_opts o)
    : w_(wal::make_wal(type, std::move(o))) {}
  inline seastar::future<wal_write_reply> append(wal_write_request req) final {
    return w_->append(std::move(req));
  }
  inline seastar::future<> invalidate(wal_write_invalidation entry) final {
    return w_->invalidate(entry);
  }
  inline seastar::future<wal_read_reply> get(wal_read_request req) final {
    return w_->get(std::move(req));
  }
  inline seastar::future<> open() final { return w_->open(); }
  inline seastar::future<> close() final { return w_->close(); }
  // seastar shardable template param
  seastar::future<> stop() { return close(); }

 private:
  std::unique_ptr<wal> w_;
};

// nicer type
using write_ahead_log = shardable_wal;

}  // namespace smf
