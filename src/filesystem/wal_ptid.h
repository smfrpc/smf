// Copyright 2018 Alexander Gallego
//

#pragma once

#include <core/sstring.hh>

#include "hashing/hashing_utils.h"

namespace smf {
/// \brief compute partition-topic-index id
inline uint64_t
wal_ptid_gen_id(const seastar::sstring &topic, const uint32_t &partition) {
  incremental_xxhash64 inc;
  inc.update(topic.data(), topic.size());
  inc.update((char *)&partition, sizeof(partition));
  return inc.digest();
}

class wal_ptid {
 public:
  wal_ptid(const seastar::sstring &topic, const uint32_t &partition)
    : id_(wal_ptid_gen_id(topic, partition)) {}
  ~wal_ptid() = default;
  wal_ptid(wal_ptid &&o) noexcept : id_(o.id_) {}
  void
  operator=(wal_ptid &&o) noexcept {
    id_ = o.id_;
  }

  inline uint64_t
  id() const {
    return id_;
  }

 private:
  friend void swap(wal_ptid &x, wal_ptid &y);
  uint64_t    id_;
};


/// \brief, helper method for maps/etc
inline void
swap(wal_ptid &x, wal_ptid &y) {
  std::swap(x.id_, y.id_);
}
inline bool
operator<(const wal_ptid &lhs, const wal_ptid &rhs) {
  return lhs.id() < rhs.id();
}
inline bool
operator==(const wal_ptid &lhs, const wal_ptid &rhs) {
  return lhs.id() == rhs.id();
}
}  // namespace smf

namespace std {
template <>
struct hash<smf::wal_ptid> {
  size_t
  operator()(const smf::wal_ptid &x) const {
    return hash<uint64_t>()(x.id());
  }
};

}  // namespace std
