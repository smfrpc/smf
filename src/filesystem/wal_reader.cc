// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_reader.h"
#include <memory>
#include <utility>
// third party
#include <core/reactor.hh>
// smf
#include "filesystem/wal_head_file_functor.h"
#include "filesystem/wal_name_extractor_utils.h"
#include "filesystem/wal_reader_node.h"
#include "hashing/hashing_utils.h"
#include "platform/log.h"


namespace smf {

wal_reader_visitor::wal_reader_visitor(wal_reader *r, seastar::file dir)
  : wal_file_walker(std::move(dir)), reader(r) {}
seastar::future<> wal_reader_visitor::visit(seastar::directory_entry de) {
  if (!wal_name_extractor_utils::is_name_locked(de.name)) {
    return reader->monitor_files(std::move(de));
  }
  return seastar::make_ready_future<>();
}

}  // namespace smf

namespace smf {
wal_reader::wal_reader(seastar::sstring workdir) : work_directory(workdir) {}

wal_reader::wal_reader(wal_reader &&o) noexcept
  : work_directory(std::move(o.work_directory))
  , allocated_(std::move(o.allocated_))
  , buckets_(std::move(o.buckets_))
  , fs_observer_(std::move(o.fs_observer_)) {}


wal_reader::~wal_reader() {}

seastar::future<> wal_reader::monitor_files(seastar::directory_entry entry) {
  auto e = wal_name_extractor_utils::extract_epoch(entry.name);
  if (buckets_.find(e) == buckets_.end()) {
    auto n =
      std::make_unique<wal_reader_node>(e, work_directory + "/" + entry.name);
    auto copy = n.get();
    return copy->open().then([this, n = std::move(n)]() mutable {
      allocated_.emplace_back(std::move(n));
      buckets_.insert(allocated_.back());
      return seastar::make_ready_future<>();
    });
  }
  return seastar::make_ready_future<>();
}

seastar::future<> wal_reader::close() {
  return seastar::do_for_each(allocated_.begin(), allocated_.end(),
                              [this](auto &b) { return b.node->close(); });
}

seastar::future<> wal_reader::open() {
  return open_directory(work_directory).then([this](seastar::file f) {
    fs_observer_ = std::make_unique<wal_reader_visitor>(this, std::move(f));
    // the visiting actually happens on a subscription thread from the filesys
    // api and it calls seastar::future<>monitor_files()...
    return fs_observer_->done();
  });
}

void wal_reader::update_file_size_by(int64_t node_epoch, uint64_t delta) {
  auto it = buckets_.lower_bound(node_epoch);
  if (it != buckets_.end()) { it->node->update_file_size_by(delta); }
}

seastar::future<wal_read_reply> wal_reader::get(wal_read_request r) {
  if (r.req->max_bytes() == 0) {
    DLOG_WARN("Received request with zero max_bytes() to read");
    return seastar::make_ready_future<wal_read_reply>();
  }
  wal_read_reply retval;
  return seastar::repeat([this, r, retval]() mutable {
           auto it = buckets_.lower_bound(r.adjusted_offset());
           if (it == buckets_.end()) {
             return seastar::make_ready_future<seastar::stop_iteration>(
               seastar::stop_iteration::yes);
           }
           if (r.adjusted_offset() < it->node->starting_epoch
               || r.adjusted_offset() >= it->node->ending_epoch()) {
             r.delta_skipped +=
               it->node->ending_epoch() - it->node->starting_epoch;
             return seastar::make_ready_future<seastar::stop_iteration>(
               seastar::stop_iteration::no);
           }
           return it->node->get(r)
             .then([this, retval, r](auto partial_reply) mutable {
               retval.merge(std::move(partial_reply));
               if (retval.size() >= r.req->max_bytes()) {
                 return seastar::stop_iteration::yes;
               }
               return seastar::stop_iteration::no;
             })
             .handle_exception([r](auto eptr) {
               LOG_ERROR("Caught exception: {}. Offset:{}, req size:{}", eptr,
                         r.adjusted_offset(), r.req->max_bytes());
               return seastar::stop_iteration::yes;
             });
         })
    .then(
      [retval] { return seastar::make_ready_future<wal_read_reply>(retval); });
}


}  // namespace smf
