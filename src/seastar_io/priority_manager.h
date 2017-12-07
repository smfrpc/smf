// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// seastar
#include <core/distributed.hh>
#include <core/future.hh>
#include <core/reactor.hh>
/// brief - inspired by the priority manager of scylla
namespace smf {
class priority_manager {
 public:
  priority_manager()
    : commitlog_priority_(
        seastar::engine().register_one_priority_class("commitlog", 100))
    , brissa_query_priority_(
        seastar::engine().register_one_priority_class("brissa_query", 100))
    , compaction_priority_(
        seastar::engine().register_one_priority_class("compaction", 100))
    , stream_read_priority_(
        seastar::engine().register_one_priority_class("streaming_read", 20))
    , stream_write_priority_(
        seastar::engine().register_one_priority_class("streaming_write", 20))
    , default_priority_(seastar::engine().register_one_priority_class(
        "smf::defult_priority", 1)) {}

  const seastar::io_priority_class &
  commitlog_priority() {
    return commitlog_priority_;
  }
  const seastar::io_priority_class &
  brissa_query_priority() {
    return brissa_query_priority_;
  }
  const seastar::io_priority_class &
  compaction_priority() {
    return compaction_priority_;
  }
  const seastar::io_priority_class &
  streaming_read_priority() {
    return stream_read_priority_;
  }
  const seastar::io_priority_class &
  streaming_write_priority() {
    return stream_write_priority_;
  }
  const seastar::io_priority_class &
  default_priority() {
    return default_priority_;
  }

  static priority_manager &
  thread_local_instance() {
    static thread_local priority_manager pm = priority_manager();
    return pm;
  }
  static priority_manager &
  get() {
    return priority_manager::thread_local_instance();
  }

 private:
  // high
  seastar::io_priority_class commitlog_priority_;
  seastar::io_priority_class brissa_query_priority_;
  seastar::io_priority_class compaction_priority_;
  // low
  seastar::io_priority_class stream_read_priority_;
  seastar::io_priority_class stream_write_priority_;
  // lowest
  seastar::io_priority_class default_priority_;
};


}  // namespace smf
