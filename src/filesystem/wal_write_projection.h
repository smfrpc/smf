#pragma once

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
  std::list<seastar::lw_shared_ptr<fbs_typed_buf<wal::tx_get_fragment>>>
    projection;
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_write_projection);

  static seastar::lw_shared_ptr<wal_write_projection> translate(
    wal::tx_put_request *req) {
    // table tx_put_request {
    // topic:       string;
    // partition:   uint; // xxhash32(topic,key)
    // txs:         [tx_put_fragment];
    // }
  }
  static seastar::lw_shared_ptr<wal_write_projection> translate(
    wal::tx_put_invalidation *req) {
    // table tx_put_request {
    // topic:       string;
    // partition:   uint; // xxhash32(topic,key)
    // txs:         [tx_put_fragment];
    // }
  }
};

}  // namespace smf
