// Copyright 2017 Alexander Gallego
//
#include "filesystem/wal_pretty_print_utils.h"


#include <flatbuffers/minireflect.h>

namespace std {
using namespace smf;       // NOLINT
using namespace smf::wal;  // NOLINT
ostream &
operator<<(ostream &o, const wal_header &h) {
  o << "wal_header={ size=" << h.size() << ", checksum=" << h.checksum()
    << ", compression=" << EnumNamewal_entry_compression_type(h.compression())
    << "}";
  return o;
}

ostream &
operator<<(ostream &o, const ::smf::wal::tx_put_fragment &f) {
  o << "tx_put_fragment={ epoch_ms=" << f.epoch_ms()
    << ", type=" << EnumNametx_put_fragment_type(f.type())
    << ", op=" << EnumNametx_put_operation(f.op())
    << ", key= " << f.key()->size() << "bytes , value=" << f.value()->size()
    << "bytes, invalidation_reason"
    << EnumNametx_put_invalidation_reason(f.invalidation_reason())
    << ", invalidation_offset=" << f.invalidation_offset() << "}";
  return o;
}


ostream &
operator<<(ostream &o, const tx_get_request &r) {
  o << "tx_get_request={ topic=" << r.topic()->c_str()
    << ", partition=" << r.partition() << ", offset=" << r.offset()
    << ", max_bytes=" << r.max_bytes() << " }";
  return o;
}
ostream &
operator<<(ostream &o, const tx_get_reply &r) {
  o << "tx_get_reply={ next_offset=" << r.next_offset() << ", gets=("
    << r.gets()->size() << "): [";
  for (const auto &g : *r.gets()) {
    o << "{ hdr: " << g->hdr() << ", fragment (size): " << g->fragment()->size()
      << " }";
  }
  o << "] }";
  return o;
}
ostream &
operator<<(ostream &o, const tx_put_request &r) {
  o << "tx_put_request={ topic=" << r.topic()->c_str() << ", data("
    << r.data()->size() << "): [";
  for (const auto &g : *r.data()) { o << g; }
  o << "]}";
  return o;
}
ostream &
operator<<(ostream &o, const tx_put_reply &r) {
  o << "tx_put_reply={ offsets=(" << r.offsets()->size() << "): [";
  for (const auto &g : *r.offsets()) { o << g; }
  o << "]}";
  return o;
}

ostream &
operator<<(ostream &o, const wal_read_request &r) {
  o << "wal_read_request={ address: " << r.req << ", req=" << *r.req << "}";
  return o;
}
ostream &
operator<<(ostream &o, const wal_read_reply &r) {
  o << "wal_read_reply={ address: " << r.reply()
    << ", errno=" << smf::wal::EnumNamewal_read_errno(r.reply()->error)
    << ", data.next_offset: " << r.reply()->next_offset << ", data.gets("
    << r.reply()->gets.size() << "): [";
  for (const auto &g : r.reply()->gets) {
    o << "{ hdr: " << *g->hdr << ", fragment (size): " << g->fragment.size()
      << " }";
  }
  o << "], consume_offset=" << r.get_consume_offset() << " }";
  return o;
}
ostream &
operator<<(ostream &o, const wal_write_request &r) {
  o << "wal_write_request={ address: " << r.req << ", req=" << *r.req << "}";
  return o;
}
ostream &
operator<<(ostream &o, const wal_write_reply &r) {
  o << "wal_write_reply={ offsets(" << r.size() << "): [";
  if (!r.empty()) {
    for (const auto &g : r) { o << *g.second; }
  }
  o << "] }";
  return o;
}

ostream &
operator<<(ostream &o, const tx_put_reply_partition_tuple &t) {
  o << "tx_put_reply_partition_tuple={ topic=" << t.topic()->c_str()
    << ", partition=" << t.partition() << ", start_offset=" << t.start_offset()
    << ", end_offset=" << t.end_offset() << " }";
  return o;
}
ostream &
operator<<(ostream &o, const tx_put_reply_partition_tupleT &t) {
  o << "tx_put_reply_partition_tupleT={ topic=" << t.topic
    << ", partition=" << t.partition << ", start_offset=" << t.start_offset
    << ", end_offset=" << t.end_offset << " }";
  return o;
}


}  // namespace std
