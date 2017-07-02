// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <memory>
#include <unordered_map>

#include <core/shared_ptr.hh>

#include "filesystem/wal_opts.h"
#include "filesystem/wal_partition_manager.h"
#include "filesystem/write_ahead_log.h"
#include "platform/macros.h"


namespace smf {
namespace wal_impl_details {
class partition_manager_list {
 public:
  partition_manager_list(const wal_opts &o, const seastar::sstring &t)
    : topic(t), opts(o) {}

  using ptr_t = seastar::lw_shared_ptr<wal_partition_manager>;

  seastar::future<ptr_t> get_manager(uint32_t partition) {
    if (pmanagers_.find(partition) == pmanagers_.end()) {
      return serialize_open_.wait(1)
        .then([this, partition] {
          if (pmanagers_.find(partition) == pmanagers_.end()) {
            auto p = seastar::make_lw_shared<wal_partition_manager>(opts, topic,
                                                                    partition);
            return p->open().then([this, partition, p] {
              // must return when fully open - otherwise you get data races
              pmanagers_.insert({partition, p});
              return seastar::make_ready_future<ptr_t>(p);
            });
          } else {
            return seastar::make_ready_future<ptr_t>(pmanagers_.at(partition));
          }
        })
        .finally([this] { serialize_open_.signal(1); });
    }
    return seastar::make_ready_future<ptr_t>(pmanagers_.at(partition));
  }
  seastar::future<> close() {
    return seastar::do_for_each(pmanagers_.begin(), pmanagers_.end(),
                                [](auto &pm) { return pm.second->close(); });
  }
  const seastar::sstring topic;
  const wal_opts &       opts;

  SMF_DISALLOW_COPY_AND_ASSIGN(partition_manager_list);


 private:
  std::unordered_map<uint32_t, seastar::lw_shared_ptr<wal_partition_manager>>
                     pmanagers_{};
  seastar::semaphore serialize_open_{1};
};

class topics_manager {
 public:
  explicit topics_manager(const wal_opts &o) : opts(o) {}

  using ptr_t = typename partition_manager_list::ptr_t;

  seastar::future<ptr_t> get_manager(const seastar::sstring &topic,
                                     uint32_t                partition) {
    if (mngrs_.find(topic) == mngrs_.end()) {
      return serialize_open_.wait(1)
        .then([this, topic, partition] {
          if (mngrs_.find(topic) == mngrs_.end()) {
            auto p   = std::make_unique<partition_manager_list>(opts, topic);
            auto raw = p.get();
            return raw->get_manager(partition).finally([
              this, topic, p = std::move(p)
            ]() mutable { mngrs_.emplace(topic, std::move(p)); });
          }
          return mngrs_.at(topic)->get_manager(partition);
        })
        .finally([this] { serialize_open_.signal(1); });
    }
    return mngrs_.at(topic)->get_manager(partition);
  }
  seastar::future<> close() {
    return seastar::do_for_each(mngrs_.begin(), mngrs_.end(),
                                [](auto &pm) { return pm.second->close(); });
  }

  const wal_opts &opts;

 private:
  std::unordered_map<seastar::sstring, std::unique_ptr<partition_manager_list>>
                     mngrs_;
  seastar::semaphore serialize_open_{1};
};
}  // namespace wal_impl_details
}  // namespace smf

namespace smf {
class wal_impl_with_cache : public write_ahead_log {
 public:
  explicit wal_impl_with_cache(wal_opts opts);
  virtual ~wal_impl_with_cache() {}

  virtual seastar::future<wal_write_reply> append(wal_write_request r) final;
  virtual seastar::future<wal_read_reply> get(wal_read_request r) final;
  virtual seastar::future<> open() final;
  virtual seastar::future<> close() final;

  const wal_opts opts;

  SMF_DISALLOW_COPY_AND_ASSIGN(wal_impl_with_cache);

 private:
  smf::wal_impl_details::topics_manager tm_;
};
}  // namespace smf
