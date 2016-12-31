#include "filesystem/wal_reader_node.h"
// third party
// smf
#include "log.h"

namespace smf {
wal_reader_node::wal_reader_node(sstring _filename, reader_stats *s)
  : filename(_filename), rstats_(DTHROW_IFNULL(s)) {}
wal_reader_node::~wal_reader_node() {}


future<> wal_reader_node::close() { return make_ready_future<>(); }
future<> wal_reader_node::open() { return make_ready_future<>(); }
future<wal_opts::maybe_buffer> wal_reader_node::get(uint64_t epoch) {
  return make_ready_future<wal_opts::maybe_buffer>();
}

} // namespace smf
