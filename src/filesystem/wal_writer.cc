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

wal_writer::wal_writer(wal_opts _opts, seastar::sstring writer_dir)
  : opts(std::move(_opts)), writer_directory(std::move(writer_dir)) {}

wal_writer::wal_writer(wal_writer &&o) noexcept
  : opts(std::move(o.opts))
  , writer_directory(std::move(o.writer_directory))
  , writer_(std::move(o.writer_)) {}

wal_writer_node_opts
wal_writer::default_writer_opts() const {
  return wal_writer_node_opts(
    opts, writer_directory, priority_manager::get().streaming_write_priority());
}

seastar::future<>
wal_writer::close() {
  if (writer_) { return writer_->close(); }
  // writer can be null if after creation, but before open()
  // is called there is a system_error or an exception
  // so we never call open. We still need to wind down and close()
  // or attempt to close all
  return seastar::make_ready_future<>();
}

seastar::future<>
wal_writer::open_empty_dir() {
  writer_ = std::make_unique<wal_writer_node>(default_writer_opts());
  return writer_->open();
}

seastar::future<>
wal_writer::open_non_empty_dir(seastar::sstring last_file) {
  auto epoch = wal_name_extractor_utils::extract_epoch(last_file);
  return seastar::file_size(writer_directory + "/" + last_file)
    .then([this, epoch](uint64_t size) {
      auto wo  = default_writer_opts();
      wo.epoch = epoch + size;
      LOG_ERROR_IF(
        size == 0,
        "Empty file size"
        "Directory was not empty. Starting at 0.wal again. Likely an "
        "incorrect shutdown behavior. You might have lost data");
      writer_ = std::make_unique<wal_writer_node>(std::move(wo));
      return writer_->open();
    });
}

seastar::future<>
wal_writer::open() {
  return seastar::open_directory(writer_directory)
    .then([this](seastar::file f) {
      auto l = seastar::make_lw_shared<wal_head_file_max_functor>(std::move(f));
      return l->done()
        .then([l, this]() {
          LOG_INFO(
            "Finished scanning the directory `{}` Last wal file found {}",
            writer_directory, l->last_file.empty() ? "(none)" : l->last_file);
          if (l->last_file.empty()) { return open_empty_dir(); }
          return open_non_empty_dir(l->last_file);
        })
        .finally([l] {});
    });
}

seastar::future<seastar::lw_shared_ptr<wal_write_reply>>
wal_writer::append(const smf::wal::tx_put_partition_tuple *it) {
  DLOG_THROW_IF(writer_ == nullptr, "cannot write with a null writer");
  return writer_->append(it);
}


}  // namespace smf
