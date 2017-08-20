#pragma once

#include "platform/macros.h"
#include "wal/wal.h"

namespace smf {
class wal_partition_manager : public wal_iface {
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
  std::unique_ptr<wal_writer>    writer_ = nullptr;

  // TODO(agallego)
  // wal reader can have a size that tells it that the size has increased by X ammount
  // on the file itself, so it can readjust the reading side, ye?
  std::unique_ptr<wal_reader>    reader_ = nullptr;

  // the cache should be the size of the write-behind log of the actual file.
  //
  std::unique_ptr<wal_mem_cache> cache_  = nullptr;
};
}
/*


so currently we do this for writes.

write to file (wal_writer_node), then update the reader size via

THEN

Update the cache. Note that we keep 1MB of write behind data before flushing to disk
SO we MUST take this into account before we yield true. i.e.: we have to keep
1MB of data in memory that the readers and the cache in the readers cannot cache
since well.. it simply can't guarantee it. ...

alternatively we can just block or smth, but that's just ugly

THEN
update_file_size_by( some bytes of THIS write).

  we need a

  struct wal_disk_projection {

     array < tx_get_fragment > write_to_read (tx_put_request)

     the key is that i know exactly how big each write is going to be.
     guaranteed. no more, no less.

     how to get that optimization
     pointer to the typename and

     // see how to use this everywhere
     a flatbuffers::DetachedBuffer{}
  }

 */
