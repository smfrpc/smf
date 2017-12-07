// Copyright 2017 Alexander Gallego
//

#pragma once

#include <cstdint>
#include <list>

#include <core/byteorder.hh>
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
  struct item {
    using buf_t = seastar::temporary_buffer<char>;
    SMF_DISALLOW_COPY_AND_ASSIGN(item);

    item(wal::wal_header header, buf_t payload)
      : hdr(std::move(header)), fragment(std::move(payload)) {}

    const wal::wal_header hdr;
    const buf_t           fragment;

    buf_t
    create_disk_header() const {
      buf_t ret(9);
      std::memset(ret.get_write(), 0, 9);
      seastar::write_le<uint32_t>(ret.get_write(), hdr.size());
      seastar::write_le<uint32_t>(ret.get_write() + 4, hdr.checksum());
      seastar::write_le<int8_t>(ret.get_write() + 8,
                                static_cast<int8_t>(hdr.compression()));
      return std::move(ret);
    }
    inline uint32_t
    on_disk_size() const {
      return 9 + fragment.size();
    }
  };

  wal_write_projection(const seastar::sstring &t, uint32_t p)
    : topic(t), partition(p) {}
  const seastar::sstring                    topic;
  const uint32_t                            partition;
  std::vector<seastar::lw_shared_ptr<item>> projection;
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
    const seastar::sstring &topic, const wal::tx_put_partition_tuple *req);
};

}  // namespace smf
