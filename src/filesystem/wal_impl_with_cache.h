// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <memory>
#include <unordered_map>

#include "filesystem/wal_opts.h"
#include "filesystem/wal_partition_manager.h"
#include "filesystem/write_ahead_log.h"

namespace smf {
class wal_impl_with_cache : public write_ahead_log {
 public:
  struct topic_parition {
    topic_parition(seastar::sstring t, uint32_t p) : topic(t), partition(p) {}
    const sstring  topic;
    const uint32_t partition;
    bool operator<(const topic_partition &rhs) const {
      return topic < rhs.topic && partition < rhs.partition;
    }
    bool operator>(const topic_partition &rhs) const {
      return topic > rhs.topic && partition > rhs.partition;
    }
    bool operator==(const topic_partition &rhs) const {
      return topic == rhs.topic && partition == rhs.partition;
    }
    bool operator!=(const topic_partition &rhs) const {
      return !(*this == rhs);
    }
    bool operator<=(const topic_partition &rhs) const {
      return (*this < rhs) || (*this == rhs);
    }
    bool operator>=(const topic_partition &rhs) const {
      return (*this > rhs) || (*this == rhs);
    }
  };

  explicit wal_impl_with_cache(wal_opts opts);
  virtual ~wal_impl_with_cache() {}

  virtual seastar::future<wal_write_reply> append(wal_write_request r) final;
  virtual seastar::future<wal_read_reply> get(wal_read_request r) final;
  virtual seastar::future<> open() final;
  virtual seastar::future<> close() final;

  const wal_opts opts;

 private:
  std::unordered_map<topic_partition, std::unique_ptr<wal_partition_manager>>
    parition_map_{};
};
}  // namespace smf
