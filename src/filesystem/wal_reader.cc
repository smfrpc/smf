// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_reader.h"
#include <memory>
#include <utility>
// third party
#include <core/reactor.hh>
#include <core/sstring.hh>
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
wal_reader::wal_reader(seastar::sstring topic, uint32_t topic_partition)
  : directory(topic), partition(topic_partition) {}

wal_reader::wal_reader(wal_reader &&o) noexcept
  : directory(std::move(o.directory))
  , partition(std::move(o.partition))
  , allocated_(std::move(o.allocated_))
  , buckets_(std::move(o.buckets_))
  , fs_observer_(std::move(o.fs_observer_)) {}


wal_reader::~wal_reader() {}

seastar::future<> wal_reader::monitor_files(seastar::directory_entry entry) {
  auto e = wal_name_extractor_utils::extract_epoch(entry.name);
  if (buckets_.find(e) == buckets_.end()) {
    auto n    = std::make_unique<wal_reader_node>(e, entry.name);
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
  return open_directory(directory).then([this](seastar::file f) {
    fs_observer_ = std::make_unique<wal_reader_visitor>(this, std::move(f));
    // the visiting actually happens on a subscription thread from the filesys
    // api and it calls seastar::future<>monitor_files()...
    return fs_observer_->done();
  });
}
void update_file_size_by(int64_t node_epoch, uint64_t delta) {
  auto it = buckets_.lower_bound(node_epoch);
  if (it != buckets_.end()) {
    it->node->update_file_size_by(delta);
  }
}

seastar::future<wal_read_reply::maybe> wal_reader::get(wal_read_request r) {
  if (r.size == 0) {
    return seastar::make_ready_future<wal_read_reply::maybe>();
  }
  auto ret = seastar::make_lw_shared<wal_read_reply>();
  auto req = seastar::make_lw_shared<wal_read_request>(r);
  return seastar::repeat([this, req, ret] {
           auto it = buckets_.lower_bound(req->offset);
           if (it == buckets_.end()) {
             return seastar::make_ready_future<seastar::stop_iteration>(
               seastar::stop_iteration::yes);
           }
           if (req->offset < it->node->starting_epoch
               || req->offset >= it->node->ending_epoch()) {
             // skip some missing files / indexes
             req->offset += it->node->ending_epoch();
             req->size -= it->node->ending_epoch();
             return seastar::make_ready_future<seastar::stop_iteration>(
               seastar::stop_iteration::no);
           }
           const auto end_epoch = it->node->ending_epoch();
           return it->node->get(*req.get())
             .then([this, ret, end_epoch, req](auto maybe) mutable {
               req->offset += end_epoch;
               req->size -= end_epoch;
               if (maybe) { ret->emplace_back(maybe->move_fragments()); }
               if (req->size <= 0) { return seastar::stop_iteration::yes; }
               return seastar::stop_iteration::no;
             })
             .handle_exception([req](auto eptr) {
               LOG_ERROR("Caught exception: {}. Offset:{}, size:{}", eptr,
                         req->offset, req->size);
               return seastar::stop_iteration::yes;
             });
         })
    .then([ret] {
      if (!ret->fragments.empty()) {
        wal_read_reply r;
        r.emplace_back(ret->move_fragments());
        return seastar::make_ready_future<wal_read_reply::maybe>(std::move(r));
      }
      return seastar::make_ready_future<wal_read_reply::maybe>();
    });
}


}  // namespace smf
