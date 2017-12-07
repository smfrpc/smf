// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_topics_manager.h"

#include <memory>
#include <utility>

#include <core/reactor.hh>


namespace smf {
using ptr_t = seastar::lw_shared_ptr<wal_partition_manager>;


partition_manager_list::partition_manager_list(const wal_opts &        o,
                                               const seastar::sstring &t)
  : topic(t), opts(o) {}

seastar::future<ptr_t> partition_manager_list::get_manager(uint32_t partition) {
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
seastar::future<> partition_manager_list::close() {
  return seastar::do_for_each(pmanagers_.begin(), pmanagers_.end(),
                              [](auto &pm) { return pm.second->close(); });
}

wal_topics_manager::wal_topics_manager(const wal_opts &o) : opts(o) {}

seastar::future<ptr_t> wal_topics_manager::get_manager(
  const seastar::sstring &topic, uint32_t partition) {
  if (mngrs_.find(topic) == mngrs_.end()) {
    return serialize_open_.wait(1)
      .then([this, topic, partition] {
        if (mngrs_.find(topic) == mngrs_.end()) {
          auto p = seastar::make_lw_shared<partition_manager_list>(opts, topic);
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
seastar::future<> wal_topics_manager::close() {
  return seastar::do_for_each(mngrs_.begin(), mngrs_.end(),
                              [](auto &pm) { return pm.second->close(); });
}


}  // namespace smf
