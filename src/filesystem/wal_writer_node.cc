// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_writer_node.h"

#include <utility>
// third party
#include <core/file.hh>
#include <core/fstream.hh>
#include <core/metrics.hh>
#include <core/reactor.hh>
// smf
#include "filesystem/wal_writer_file_lease.h"
#include "filesystem/wal_writer_utils.h"
#include "hashing/hashing_utils.h"
#include "platform/log.h"
#include "platform/macros.h"

namespace smf {

static const size_t kWalHeaderSize = sizeof(wal::wal_header);
static uint64_t     wal_write_request_size(
  seastar::lw_shared_ptr<wal_write_projection> req) {
  return std::accumulate(
    req->projection.begin(), req->projection.end(), uint64_t(0),
    [](uint64_t acc, const auto &it) { return acc + it->size(); });
}

wal_writer_node::wal_writer_node(wal_writer_node_opts &&opts)
  : opts_(std::move(opts)) {
  namespace sm = seastar::metrics;
  metrics_.add_group(
    "smf::wal_writer_node::" + opts_.topic + " ("
      + seastar::to_sstring(opts_.partition) + ")",
    {sm::make_derive("total_writes", stats_.total_writes,
                     sm::description("Number of writes to disk")),
     sm::make_derive("total_bytes", stats_.total_bytes,
                     sm::description("Number of bytes writen to disk")),
     sm::make_derive("total_rolls", stats_.total_rolls,
                     sm::description("Number of times, we rolled the log"))});
}
wal_writer_node::~wal_writer_node() {}

seastar::future<> wal_writer_node::open() {
  const auto name =
    wal_file_name(opts_.work_directory, opts_.run_id, opts_.epoch);
  DLOG_TRACE("Rolling log: {}", name);
  LOG_THROW_IF(lease_, "opening new file. Previous file is unclosed");
  ++stats_.total_rolls;
  lease_ =
    seastar::make_lw_shared<smf::wal_writer_file_lease>(name, opts_.fstream);
  return lease_->open();
}
seastar::future<> wal_writer_node::disk_write(
  seastar::lw_shared_ptr<wal_write_projection::item> frag) {
  stats_.total_bytes += frag->hdr.buf().size();
  return lease_->append(frag->hdr.buf().get(), frag->hdr.buf().size())
    .then([frag, this] {
      current_size_ += frag->fragment.size();
      stats_.total_bytes += frag->fragment.size();
      ++stats_.total_writes;
      return lease_->append(frag->fragment.get(), frag->fragment.size());
    });
}

seastar::future<wal_write_reply> wal_writer_node::append(
  seastar::lw_shared_ptr<wal_write_projection> req) {
  return serialize_writes_.wait(1)
    .then([this, req]() mutable {
      auto     start_offset = current_offset();
      uint64_t write_size   = wal_write_request_size(req);
      return seastar::do_for_each(
               req->projection.begin(), req->projection.end(),
               [this](auto &i) mutable { return this->do_append(i); })
        .then([write_size, start_offset] {
          return seastar::make_ready_future<wal_write_reply>(
            wal_write_reply(start_offset, start_offset + write_size));
        });
    })
    .finally([this] { serialize_writes_.signal(1); });
}


seastar::future<> wal_writer_node::do_append(
  seastar::lw_shared_ptr<wal_write_projection::item> frag) {
  if (SMF_LIKELY(frag->size() <= space_left())) { return disk_write(frag); }
  return rotate_fstream().then(
    [this, frag]() mutable { return disk_write(frag); });
}

seastar::future<> wal_writer_node::close() {
  // need to make sure the file is not closed in the midst of a write
  //
  return serialize_writes_.wait(1)
    .then([l = lease_] {
      return l->flush().then([l] { return l->close(); }).finally([l] {});
    })
    .finally([this] { serialize_writes_.signal(1); });
}
seastar::future<> wal_writer_node::rotate_fstream() {
  DLOG_INFO("rotating fstream");
  // Although close() does similar work, it will deadlock the fiber
  // if you call close here. Close ensures that there is no other ongoing
  // operations and it is a public method which needs to serialize access to
  // the internal file.
  auto l = lease_;
  return l->flush()
    .then([l] { return l->close(); })
    .then([this] {
      lease_ = nullptr;
      opts_.epoch += current_size_;
      current_size_ = 0;
      return open();
    })
    .finally([l] {});
}


}  // namespace smf
