#pragma once

#include "filesystem/wal_reader.h"
#include "filesystem/wal_write_behind_cache.h"
#include "filesystem/wal_writer.h"
#include "filesystem/write_ahead_log.h"
#include "platform/macros.h"

namespace smf {
class wal_partition_manager : public write_ahead_log {
 public:
  wal_partition_manager(wal_opts otps,
                        sstring  topic_name,
                        uint32_t topic_partition);
  wal_partition_manager(wal_partition_manager &&o) noexcept;
  virtual ~wal_partition_manager();

  virtual seastar::future<wal_write_reply> append(wal_write_request r) final;
  virtual seastar::future<wal_read_reply> get(wal_read_request r) final;
  virtual seastar::future<> open() final;
  virtual seastar::future<> close() final;


  SMF_DISALLOW_COPY_AND_ASSIGN(wal_partition_manager);

 public:
  const wal_opts opts;
  const uint32_t topic;
  const uint32_t partition;

 private:
  std::unique_ptr<wal_writer>             writer_ = nullptr;
  std::unique_ptr<wal_reader>             reader_ = nullptr;
  std::unique_ptr<wal_write_behind_cache> cache_  = nullptr;
};

}  // namespace smf
