// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <memory>
// smf
#include "filesystem/wal_mem_cache.h"
#include "filesystem/wal_reader.h"
#include "filesystem/wal_writer.h"
#include "filesystem/write_ahead_log.h"

namespace smf {
class wal_impl_with_cache : public write_ahead_log {
 public:
  explicit wal_impl_with_cache(wal_opts opts);
  virtual ~wal_impl_with_cache() {}

  virtual seastar::future<wal_write_reply> append(wal_write_request r) final;
  virtual seastar::future<> invalidate(wal_write_invalidation r) final;
  virtual seastar::future<wal_read_reply> get(wal_read_request r) final;
  virtual seastar::future<> open() final;
  virtual seastar::future<> close() final;


 private:
  std::unordered_map<uint32_t, std::unique_ptr<wal_partition_manager>>
    parition_map_{};
};
}  // namespace smf
