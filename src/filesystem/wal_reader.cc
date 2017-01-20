// source header
#include "filesystem/wal_reader.h"
// third party
#include <core/reactor.hh>
// smf
#include "filesystem/wal_head_file_functor.h"
#include "filesystem/wal_name_extractor_utils.h"
#include "filesystem/wal_reader_node.h"
#include "log.h"


namespace smf {
wal_reader_visitor::wal_reader_visitor(wal_reader *r, file dir, sstring prefix)
  : wal_file_walker(std::move(dir), std::move(prefix)), reader(r) {}

future<> wal_reader_visitor::visit(directory_entry wal_file_entry) {
  return reader->monitor_files(std::move(wal_file_entry));
}

} // namespace smf

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
  auto e = extract_epoch(entry.name);
  if(buckets_.find(e) == buckets_.end()) {
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

future<wal_opts::maybe_buffer> wal_reader::get(wal_read_request r) {
  auto it = buckets_.lower_bound(r.offset);
  if(it != buckets_.end()) {
    if(r.offset >= it->node->starting_epoch
       && r.offset <= it->node->ending_epoch()) {
      return it->node->get(std::move(r));
    }
  }
  return make_ready_future<wal_opts::maybe_buffer>();
}


} // namespace smf
