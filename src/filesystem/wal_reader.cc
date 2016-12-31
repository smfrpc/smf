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
    readers_(std::move(o.readers_)) {}


wal_reader::~wal_reader() {}

future<> monitor_files(directory_entry entry) {
  auto e = extract_epoch(entry.name);
  if(reader->readers_.find(e) == reader->readers_.end()) {
    reader->readers_.insert({e, std::make_unique<wal_reader_node>(entry.name)});
  }
}

future<> wal_reader::close() {
  assert(reader_ != nullptr);
  return reader_->close();
}

future<> wal_reader::open() {
  return open_directory(directory).then([this](file f) {
    fs_observer = std::make_unique<wal_reader_visitor>(this, std::move(f));
    // the visiting actually happens on a subscription thread from the filesys
    // api and it calls future<>monitor_files()...
    return make_ready_future<>();
  });
}

future<wal_opts::maybe_buffer> wal_reader::get(wal_read_request r) {
  return make_ready_future<wal_opts::maybe_buffer>();
}


} // namespace smf
