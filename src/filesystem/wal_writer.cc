// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
// source header
#include "filesystem/wal_writer.h"
#include <algorithm>
#include <memory>
#include <utility>
// third party
#include <core/reactor.hh>
// smf
#include "filesystem/wal_head_file_functor.h"
#include "filesystem/wal_name_extractor_utils.h"
#include "filesystem/wal_writer_node.h"
#include "filesystem/wal_writer_utils.h"
#include "platform/log.h"
#include "seastar_io/priority_manager.h"
#include "utils/time_utils.h"

namespace smf {

wal_writer::wal_writer(sstring _dir, writer_stats *s)
  : directory(_dir), wstats_(s) {}

wal_writer::wal_writer(wal_writer &&o) noexcept
  : directory(std::move(o.directory)),
    wstats_(o.wstats_),
    writer_(std::move(o.writer_)) {}


future<> wal_writer::close() {
  assert(writer_ != nullptr);
  return writer_->close();
}

future<> wal_writer::open_empty_dir(sstring prefix) {
  auto                 id = to_sstring(engine().cpu_id());
  wal_writer_node_opts wo;
  wo.wstats = wstats_;
  wo.prefix = prefix + id;
  writer_   = std::make_unique<wal_writer_node>(std::move(wo));
  return writer_->open();
}

future<> wal_writer::open_non_empty_dir(sstring last_file, sstring prefix) {
  auto epoch = wal_name_extractor_utils::extract_epoch(last_file);
  DLOG_TRACE("epoch extracted: {}, filename:{}", epoch, last_file);
  return open_file_dma(last_file, open_flags::ro)
    .then([this, prefix, epoch](file f) {
      auto last = make_lw_shared<file>(std::move(f));
      return last->size().then([this, prefix, last, epoch](uint64_t size) {
        auto                 id = to_sstring(engine().cpu_id());
        wal_writer_node_opts wo;
        wo.wstats = wstats_;
        wo.prefix = prefix + id;
        wo.epoch  = epoch + size;
        writer_   = std::make_unique<wal_writer_node>(std::move(wo));
        return last->close().then([last, this] { return writer_->open(); });
      });
    });
}

future<> wal_writer::do_open() {
  return open_directory(directory).then([this](file f) {
    auto l = make_lw_shared<wal_head_file_max_functor>(std::move(f));
    return l->close().then([l, this]() {
      sstring run_prefix = to_sstring(time_now_micros()) + ":";
      if (l->last_file.empty()) {
        return open_empty_dir(run_prefix);
      }
      return open_non_empty_dir(l->last_file, run_prefix);
    });
  });
}

future<> wal_writer::open() {
  return file_exists(directory).then([this](bool dir_exists) {
    if (dir_exists) {
      return do_open();
    }
    return make_directory(directory).then([this] { return do_open(); });
  });
}

future<uint64_t> wal_writer::append(wal_write_request req) {
  return writer_->append(std::move(req));
}

future<> wal_writer::invalidate(uint64_t epoch) {
  // write invalidation structure to WAL
  fbs::wal::invalid_wal_entry e{epoch};
  temporary_buffer<char>      data(sizeof(e));
  std::copy(reinterpret_cast<char *>(&e),
            reinterpret_cast<char *>(&e) + sizeof(e), data.get_write());

  wal_write_request req(
    wal_write_request_flags::wwrf_invalidate_payload, std::move(data),
    priority_manager::thread_local_instance().commitlog_priority());

  return append(std::move(req)).then([](auto _) {
    return make_ready_future<>();
  });
}

}  // namespace smf
