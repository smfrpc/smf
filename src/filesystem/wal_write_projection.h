// Copyright 2017 Alexander Gallego
//

#pragma once

#include <list>

#include <core/shared_ptr.hh>
#include <core/sstring.hh>

#include "flatbuffers/fbs_typed_buf.h"
#include "flatbuffers/wal_generated.h"
#include "platform/macros.h"

namespace smf {

///  \brief alternatives considered for the buf was to use the
///  flatbuffers::DetachedBuffer.
///
/// The issue w / it is that all the IO in seastar uses temporary_buffer<>'s and
/// it already has easy facilities to share the buffer so that it will cleanly
/// delete the resources behind the  scenes  So we pay the memcpy once and then
/// call the tmporary_buffer<>::share(i,j) calls to get a reference to the same
/// underlying array, minimizing memory consumption
///
struct wal_write_projection {
  // This is ALMOST identical to the wal_generated.h
  // HOWEVER - a set of major improvements - it works with seastar IO subsystem.
  // Which means we can share this all we want and we'll get the most memory
  // friendlyness
  struct item {
    static constexpr uint32_t kWalHeaderSize = sizeof(wal::wal_header);

    item() : hdr(seastar::temporary_buffer<char>(kWalHeaderSize)) {}

    fbs_typed_buf<wal::wal_header>  hdr;
    seastar::temporary_buffer<char> fragment;

    uint32_t size() { return hdr.buf().size() + fragment.size(); }
    SMF_DISALLOW_COPY_AND_ASSIGN(item);
  };

  wal_write_projection() {}
  wal_write_projection(wal_write_projection &&o) noexcept
    : projection(std::move(o.projection)) {}
  std::list<seastar::lw_shared_ptr<item>> projection{};
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_write_projection);

  /// \brief takes in what the user expected to write, and converts it to a
  /// format that will actually be flushed to disk, which may or may not
  /// include compression, encryption, etc.
  ///
  /// Note: We ignore partial-fragments. That is, say that a WAL is of size 10,
  /// and we are offset 8. The put that the user just sent us is of size 4 (a
  /// big one). We have multiple options:
  ///     a) Write a partial-fragment on the current WAL of size 2
  ///        and then write a second partial-fragment into a new roll (log
  ///        segment)
  ///
  ///     b) Allocate a log segment bigger than the largest possible payload
  ///     (2GB) and simply close this current log segment and start a new one
  ///     and write it.
  ///
  /// We chose (b) explicitly. If you look at the history of commits, you will
  /// also see an implementation which did the 'correct' version (a), and it was
  /// slow and  complex. It increases the complexity of clients and the server
  /// code does a LOT more  memcpy.
  ///
  /// Kafka also does (b). and ppl don't run into this scenario in real life too
  /// often.
  ///
  /// Usability Note: We use a write-behind queue as we are bypassing the kernel
  /// page cache.  The default size is seastar's default 'write-behind' cache of
  /// 1MB. That means that even after you write, your reads will *not* always be
  /// avail. After you write them, just keep 1MB around of the latest writes.
  /// the readers also implement a sophisticated cache with intelligent eviction
  /// schemes, but you must cache the write-behind
  ///
  static seastar::lw_shared_ptr<wal_write_projection> translate(
    const wal::tx_put_partition_pair *req);
};

}  // namespace smf
