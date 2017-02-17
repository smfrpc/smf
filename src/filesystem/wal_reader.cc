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

wal_reader_visitor::wal_reader_visitor(wal_reader *r, file dir)
  : wal_file_walker(std::move(dir)), reader(r) {}
future<> wal_reader_visitor::visit(directory_entry de) {
  if (!wal_name_extractor_utils::is_name_locked(de.name)) {
    auto original_core = wal_name_extractor_utils::extract_core(de.name);
    auto moving_core   = jump_consistent_hash(original_core, smp::count);
    // only move iff* it matches our core. Every core is doing the same
    if (engine().cpu_id() == moving_core) {
      return reader->monitor_files(std::move(de));
    }
  }
  return make_ready_future<>();
}
}  // namespace smf

namespace smf {
wal_reader::wal_reader(sstring _dir, reader_stats *s)
  : directory(_dir), rstats_(DTHROW_IFNULL(s)) {}

wal_reader::wal_reader(wal_reader &&o) noexcept
  : directory(std::move(o.directory)),
    rstats_(o.rstats_),
    allocated_(std::move(o.allocated_)),
    buckets_(std::move(o.buckets_)),
    fs_observer_(std::move(o.fs_observer_)) {}


wal_reader::~wal_reader() {}

future<> wal_reader::monitor_files(directory_entry entry) {
  auto e = wal_name_extractor_utils::extract_epoch(entry.name);
  if (buckets_.find(e) == buckets_.end()) {
    auto n = std::make_unique<wal_reader_node>(e, entry.name, rstats_);
    allocated_.emplace_back(std::move(n));
    buckets_.insert(allocated_.back());
    return allocated_.back().node->open();
  }
  return make_ready_future<>();
}

future<> wal_reader::close() {
  return do_for_each(allocated_.begin(), allocated_.end(),
                     [this](auto &b) { return b.node->close(); });
}

future<> wal_reader::open() {
  return open_directory(directory).then([this](file f) {
    fs_observer_ = std::make_unique<wal_reader_visitor>(this, std::move(f));
    // the visiting actually happens on a subscription thread from the filesys
    // api and it calls future<>monitor_files()...
    return make_ready_future<>();
  });
}

future<wal_read_reply::maybe> wal_reader::get(wal_read_request r) {
  if (r.size == 0 || r.offset == 0) {
    return make_ready_future<wal_read_reply::maybe>();
  }
  auto it = buckets_.lower_bound(r.offset);
  if (it != buckets_.end()) {
    if (r.offset >= it->node->starting_epoch) {
      return it->node->get(r).then([this, r](auto maybe) mutable {
        if (maybe) {
          auto payload_size = maybe->size();
          if (payload_size >= r.size) {
            return make_ready_future<wal_read_reply::maybe>(std::move(maybe));
          }
          r.offset += maybe->size();
          r.size -= maybe->size();
          return this->get(r).then([p = std::move(maybe)](auto next) mutable {
            auto ret = std::move(p.value());
            if (next) {
              ret.merge(std::move(next.value()));
            }
            return make_ready_future<wal_read_reply::maybe>(std::move(ret));
          });
        }
        return make_ready_future<wal_read_reply::maybe>();
      });
    }
  }
  return make_ready_future<wal_read_reply::maybe>();
}


}  // namespace smf
