#pragma once

#include "platform/macros.h"
#include "wal/write_ahead_log.h"

namespace smf {
class wal_partition_manager : public write_ahead_log {
 public:
  wal_partition_manager(wal_opts otps,
                        sstring  topic_name,
                        uint32_t topic_partition)
    : topic(topic_name), partition(topic_partition) {}
  virtual ~wal_partition_manager() {}

  virtual seastar::future<wal_write_reply> append(wal_write_request r) = 0;
  virtual seastar::future<> invalidate(wal_write_invalidation r)       = 0;
  virtual seastar::future<wal_read_reply> get(wal_read_request r)      = 0;
  virtual seastar::future<> open();
  virtual seastar::future<> close();


  SMF_DISALLOW_COPY_AND_ASSIGN(wal_partition_manager);

 public:
  const uint32_t topic;
  const uint32_t partition;

 private:
  std::unique_ptr<wal_writer> writer_ = nullptr;
  std::unique_ptr<wal_reader> reader_ = nullptr;

  // The cache should be the size of the write-behind log of the actual file.
  //
  // Note that we keep 1MB of write behind data before flushing to disk. SO we
  // MUST take this into account before we yield true. i.e.: we have to keep 1MB
  // of data in memory so you can have deterministic readers.
  //
  std::unique_ptr<wal_mem_cache> cache_ = nullptr;
};

}  // namespace smf
