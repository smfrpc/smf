// Copyright 2017 Alexander Gallego
//

#pragma once

#include "filesystem/wal_reader.h"
#include "filesystem/wal_requests.h"
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
  seastar::future<seastar::lw_shared_ptr<wal_write_reply>> append(
    const smf::wal::tx_put_partition_tuple *it);
  seastar::future<seastar::lw_shared_ptr<wal_read_reply>> get(
    wal_read_request r);

  seastar::future<> open();
  seastar::future<> close();


  SMF_DISALLOW_COPY_AND_ASSIGN(wal_partition_manager);

 public:
  const wal_opts         opts;
  const seastar::sstring topic;
  const uint32_t         partition;
  const seastar::sstring work_dir;

 private:
  seastar::future<> do_open();

 private:
  wal_writer writer_;
  wal_reader reader_;
};

}  // namespace smf
