// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <memory>
// smf
#include "filesystem/wal.h"
#include "filesystem/wal_mem_cache.h"
#include "filesystem/wal_reader.h"
#include "filesystem/wal_writer.h"
namespace smf {
class wal_impl_with_cache : public wal {
 public:
  explicit wal_impl_with_cache(wal_opts _opts);

  seastar::future<uint64_t> append(wal_write_request req) final;
  seastar::future<> invalidate(uint64_t epoch) final;

  seastar::future<> open() final;
  seastar::future<> close() final;

  seastar::future<wal_read_reply::maybe> get(wal_read_request req) final;

 private:
  std::unique_ptr<wal_writer>    writer_ = nullptr;
  std::unique_ptr<wal_reader>    reader_ = nullptr;
  std::unique_ptr<wal_mem_cache> cache_  = nullptr;
};
}  // namespace smf
