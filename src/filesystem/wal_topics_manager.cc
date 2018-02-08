// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_topics_manager.h"

#include <memory>
#include <utility>

#include <sys/sdt.h>

#include <core/reactor.hh>

namespace smf {

wal_topics_manager::wal_topics_manager(wal_opts o) : opts(o) {}

seastar::future<wal_partition_manager *>
wal_topics_manager::get_manager(seastar::sstring topic, uint32_t partition) {
  using ptr = wal_partition_manager *;
  wal_ptid idx(topic, partition);
  auto     it = mngrs_.find(idx);
  if (it == mngrs_.end()) {
    return seastar::with_semaphore(serialize_open_, 1, [
      this, topic, partition, idx = std::move(idx)
    ]() mutable {
      auto it2 = mngrs_.find(idx);
      if (it2 != mngrs_.end()) {
        return seastar::make_ready_future<ptr>(it2->second.get());
      }
      auto x = std::make_unique<wal_partition_manager>(opts, topic, partition);
      auto y = x.get();
      return y->open().then(
        [ this, y, x = std::move(x), idx = std::move(idx) ]() mutable {
          DTRACE_PROBE1(smf, partition_manager_create, idx.id());
          mngrs_.emplace(std::move(idx), std::move(x));
          return seastar::make_ready_future<ptr>(y);
        });
    });  // end semaphore
  }
  DTRACE_PROBE1(smf, partition_manager_get, idx.id());
  ptr x = it->second.get();
  return seastar::make_ready_future<ptr>(x);
}
seastar::future<>
wal_topics_manager::close() {
  return seastar::do_for_each(mngrs_.begin(), mngrs_.end(),
                              [](auto &pm) { return pm.second->close(); });
}


}  // namespace smf
