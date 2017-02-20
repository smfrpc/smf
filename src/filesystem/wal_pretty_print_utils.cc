// Copyright 2017 Alexander Gallego
//
#include "filesystem/wal_pretty_print_utils.h"

namespace std {
ostream &operator<<(ostream &o, const ::smf::fbs::wal::wal_header &h) {
  namespace fl = ::smf::fbs::wal;
  o << "{'flags':'";
  auto flags = static_cast<uint32_t>(h.flags());
  if (flags & fl::wal_entry_flags::wal_entry_flags_partial_fragment) {
    o << "(partial_fragment)";
  }
  if (flags & fl::wal_entry_flags::wal_entry_flags_full_frament) {
    o << "(full_frament)";
  }
  if (flags & fl::wal_entry_flags::wal_entry_flags_zstd) {
    o << "(zstd)";
  }
  o << "', 'size':" << h.size() << ", 'checksum':" << h.checksum() << "}";
  return o;
}

ostream &operator<<(ostream &o, const ::smf::wal_read_request &r) {
  o << "{'offset':" << r.offset << ", 'size':" << r.size
    << ", 'flags': " << r.flags << " }";
  return o;
}

}  // namespace std
