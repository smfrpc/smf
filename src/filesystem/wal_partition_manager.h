// Copyright 2017 Alexander Gallego
//

#pragma once

#include "filesystem/wal_reader.h"
#include "filesystem/wal_requests.h"
#include "filesystem/wal_write_behind_cache.h"
#include "filesystem/wal_writer.h"
#include "platform/macros.h"

namespace smf {
class wal_partition_manager {
 public:
  wal_partition_manager(wal_opts         otps,
                        seastar::sstring topic_name,
                        uint32_t         topic_partition);
  wal_partition_manager(wal_partition_manager &&o) noexcept;
  ~wal_partition_manager();

  /// \brief this is designed to ONLY handle a core's writes
  /// exclusively. Which means that if you see system_error
  /// directory_not found on your logs it means that you are
  /// not routing requests to this core exclusively. Remember
  /// to do something like this
  /// \code
  ///    if (smf::jump_consistent_hash(it->partition(), seastar::smp::count)
  ///             == seastar::engine().cpu_id()) {
  ///
  ///        partitions.insert(it->partition());
  ///    }
  /// \endcode
  seastar::future<wal_write_reply> append(
    seastar::lw_shared_ptr<wal_write_projection> projection);

  seastar::future<wal_read_reply> get(wal_read_request r);
  seastar::future<> open();
  seastar::future<> close();


  SMF_DISALLOW_COPY_AND_ASSIGN(wal_partition_manager);

 public:
  const wal_opts         opts;
  const seastar::sstring topic;
  const uint32_t         partition;

  bool is_open() const { return is_ready_open_; }

 private:
  seastar::future<> do_open();

 private:
  bool                                    is_ready_open_ = false;
  std::unique_ptr<wal_writer>             writer_        = nullptr;
  std::unique_ptr<wal_reader>             reader_        = nullptr;
  std::unique_ptr<wal_write_behind_cache> cache_         = nullptr;
};

}  // namespace smf
