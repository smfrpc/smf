// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_writer_node.h"

#include <utility>

#include <sys/sdt.h>
// third party
#include <core/file.hh>
#include <core/fstream.hh>
#include <core/metrics.hh>
#include <core/reactor.hh>
// smf
#include "filesystem/wal_segment.h"
#include "filesystem/wal_writer_utils.h"
#include "hashing/hashing_utils.h"
#include "platform/log.h"
#include "platform/macros.h"

namespace smf {

static uint64_t
wal_write_request_size(const smf::wal::tx_put_partition_tuple *it) {
  return std::accumulate(
    it->txs()->begin(), it->txs()->end(), uint64_t(0),
    [](uint64_t acc, const smf::wal::tx_put_binary_fragment *x) {
      return acc + x->data()->size();
    });
}

wal_writer_node::wal_writer_node(wal_writer_node_opts &&opts)
  : opts_(std::move(opts)) {
  namespace sm = seastar::metrics;
  metrics_.add_group(
    "smf::wal_writer_node::" + opts_.writer_directory,
    {sm::make_derive("total_writes", stats_.total_writes,
                     sm::description("Number of writes to disk")),
     sm::make_derive("total_bytes", stats_.total_bytes,
                     sm::description("Number of bytes writen to disk")),
     sm::make_derive("total_rolls", stats_.total_rolls,
                     sm::description("Number of times, we rolled the log"))});

  flush_timeout_.set_callback([this] {
    auto l = lease_;
    return seastar::with_semaphore(serialize_writes_, 1, [l] {
      DTRACE_PROBE(smf, wal_writer_node_periodic_flush);
      return l->flush();
    });
  });
  flush_timeout_.arm_periodic(opts_.wopts.writer_flush_period);
}
wal_writer_node::~wal_writer_node() {}

seastar::future<>
wal_writer_node::open() {
  const auto name = wal_file_name(opts_.writer_directory, opts_.epoch);
  DLOG_TRACE("Rolling log: {}", name);
  LOG_THROW_IF(lease_, "opening new file. Previous file is unclosed");
  ++stats_.total_rolls;
  lease_ = seastar::make_lw_shared<smf::wal_segment>(
    name, opts_.pclass, opts_.write_file_in_memory_cache,
    opts_.write_concurrency);
  return lease_->open();
}
seastar::future<>
wal_writer_node::disk_write(const smf::wal::tx_put_binary_fragment *f) {
  stats_.total_bytes += f->data()->size();
  current_size_ += f->data()->size();
  ++stats_.total_writes;
  return lease_->append((const char *)f->data()->Data(), f->data()->size());
}

seastar::future<seastar::lw_shared_ptr<wal_write_reply>>
wal_writer_node::append(const smf::wal::tx_put_partition_tuple *it) {
  DTRACE_PROBE(smf, wal_writer_node_append);
  return seastar::with_semaphore(serialize_writes_, 1, [this, it]() mutable {
    const uint64_t start_offset = current_offset();
    const uint64_t write_size   = wal_write_request_size(it);
    return seastar::do_for_each(
             it->txs()->begin(), it->txs()->end(),
             [this](auto i) mutable { return this->do_append(i); })
      .then([write_size, start_offset, it, this] {
        LOG_THROW_IF(
          start_offset + write_size != current_offset(),
          "Invalid offset accounting: start_offset:{}, "
          "write_size:{}, current_offset(): {}, total_writes_in_batch: {}",
          start_offset, write_size, current_offset(), it->txs()->size());
        auto ret = seastar::make_lw_shared<wal_write_reply>();
        ret->set_reply_partition_tuple("need_to_set", it->partition(),
                                       opts_.epoch + start_offset,
                                       opts_.epoch + start_offset + write_size);
        return seastar::make_ready_future<decltype(ret)>(ret);
      });
  });
}


seastar::future<>
wal_writer_node::do_append(const smf::wal::tx_put_binary_fragment *f) {
  if (SMF_LIKELY(f->data()->size() <= space_left())) { return disk_write(f); }
  return rotate_fstream().then([this, f]() mutable { return disk_write(f); });
}

seastar::future<>
wal_writer_node::close() {
  flush_timeout_.cancel();
  // need to make sure the file is not closed in the midst of a write
  //
  return seastar::with_semaphore(serialize_writes_, 1, [l = lease_] {
    return l->flush().then([l] { return l->close(); }).finally([l] {});
  });
}
seastar::future<>
wal_writer_node::rotate_fstream() {
  DTRACE_PROBE(smf, wal_writer_node_rotation);
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
