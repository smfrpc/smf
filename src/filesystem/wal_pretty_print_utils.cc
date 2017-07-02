// Copyright 2017 Alexander Gallego
//
#include "filesystem/wal_pretty_print_utils.h"

namespace std {
using namespace smf;       // NOLINT
using namespace smf::wal;  // NOLINT
ostream &operator<<(ostream &o, const wal_header &h) {
  o << "wal_header={compression: "
    << EnumNamewal_entry_compression_type(h.compression())
    << ", size: " << h.size() << " , checksum: " << h.checksum() << "}";
  return o;
}
ostream &operator<<(ostream &o, const tx_get_request &r) {
  o << "tx_get_request={topic: " << r.topic()
    << ", partition: " << r.partition() << ", offset: " << r.offset()
    << ", max_bytes: " << r.max_bytes() << "}";
  return o;
}
ostream &operator<<(ostream &o, const tx_get_reply &r) {
  o << "tx_get_reply={next_offset:" << r.next_offset() << " PENDING TODO }";
  return o;
}
ostream &operator<<(ostream &o, const tx_put_request &r) {
  o << "tx_put_request={ PENDING TODO }";
  return o;
}
ostream &operator<<(ostream &o, const tx_put_reply &r) {
  o << "tx_put_reply={ PENDING TODO }";
  return o;
}

ostream &operator<<(ostream &o, const wal_read_request &r) {
  o << "wal_read_request={ address: " << r.req << "}";
  return o;
}
ostream &operator<<(ostream &o, const wal_read_reply &r) {
  o << "wal_read_reply={ address: " << r.data.get() << "}";
  return o;
}
ostream &operator<<(ostream &o, const wal_write_request &r) {
  o << "wal_write_request={ address: " << r.req << "}";
  return o;
}
ostream &operator<<(ostream &o, const wal_write_reply &r) {
  o << "wal_write_reply={ start_offset: " << r.data.start_offset << "}";
  return o;
}

}  // namespace std
