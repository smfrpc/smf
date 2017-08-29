#pragma once

#include "filesystem/wal_reader.h"
#include "filesystem/wal_write_behind_cache.h"
#include "filesystem/wal_writer.h"
#include "filesystem/write_ahead_log.h"
#include "platform/macros.h"

namespace smf {
class wal_partition_manager {
 public:
  wal_partition_manager(wal_opts         otps,
                        seastar::sstring topic_name,
                        uint32_t         topic_partition);
  wal_partition_manager(wal_partition_manager &&o) noexcept;
  ~wal_partition_manager();

  seastar::future<wal_write_reply> append(
    seastar::lw_shared_ptr<wal_write_projection> projection);

  seastar::future<wal_read_reply> get(wal_read_request r);
  seastar::future<> open();
  seastar::future<> close();


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
