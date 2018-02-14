// Copyright 2018 Alexander Gallego
//


#include "filesystem/wal_requests.h"

#include "hashing/hashing_utils.h"

namespace smf {

bool
wal_read_request::valid(const wal_read_request &r) {
  try {
    // traversing the fbs might throw
    (void)r.req->partition();
    (void)r.req->offset();
    (void)r.req->max_bytes();
    return r.req->topic()->size() != 0;
  } catch (const std::exception &e) {
    LOG_ERROR("Validating wal_read_request: caught exception: {}", e.what());
  }
  return true;
}


wal_read_reply::wal_read_reply(uint64_t request_offset) {
  data_->next_offset = request_offset;
}

uint64_t
wal_read_reply::on_disk_size() {
  if (empty()) { return 0; }
  return std::accumulate(data_->gets.begin(), data_->gets.end(), uint64_t(0),
                         [](uint64_t acc, const auto &it) {
                           return acc + it->fragment.size() +
                                  kOnDiskInternalHeaderSize;
                         });
}

uint64_t
wal_read_reply::on_wire_size() {
  if (empty()) { return 0; }
  return std::accumulate(data_->gets.begin(), data_->gets.end(), uint64_t(0),
                         [](uint64_t acc, const auto &it) {
                           return acc + it->fragment.size() + kWalHeaderSize;
                         });
}


wal_write_request::wal_write_request(
  const smf::wal::tx_put_request *                ptr,
  const ::seastar::io_priority_class &            p,
  const uint32_t                                  _runner_core,
  std::vector<const smf::wal::tx_put_partition_tuple *> partitions_view)
  : priority_wrapper(ptr, p)
  , runner_core(_runner_core)
  , partitions(std::move(partitions_view)) {
  DLOG_THROW_IF(partitions.empty(), "Partition view is empty");
}

bool
wal_write_request::valid(const wal_write_request &r) {
  try {
    // traversing the request object might throw
    (void)r.req->topic();
    (void)r.req->data();
    return !r.partitions.empty() && r.req->topic()->size() != 0;
  } catch (const std::exception &e) {
    LOG_ERROR("Validating wal_write_request: caught exception: {}", e.what());
  }
  return true;
}


void
wal_write_reply::set_reply_partition_tuple(const seastar::sstring &topic,
                                           uint32_t                partition,
                                           uint64_t                begin,

                                           uint64_t end) {
  auto key = idx(topic.data(), topic.size(), partition);
  auto it  = cache_.find(key);
  if (it == cache_.end()) {
    auto p          = std::make_unique<wal::tx_put_reply_partition_tupleT>();
    p->topic        = topic;
    p->partition    = partition;
    p->start_offset = begin;
    p->end_offset   = end;
    cache_.insert({key, std::move(p)});
  } else {
    it->second->start_offset = std::max(it->second->start_offset, begin);
    it->second->end_offset   = std::max(it->second->end_offset, end);
  }
}

std::unique_ptr<smf::wal::tx_put_replyT>
wal_write_reply::move_into() {
  auto ret = std::make_unique<smf::wal::tx_put_replyT>();
  for (auto &&p : cache_) { ret->offsets.push_back(std::move(p.second)); }
  return std::move(ret);
}

uint64_t
wal_write_reply::idx(const char *data,
                     std::size_t data_sz,
                     uint32_t    partition) {
  incremental_xxhash64 inc;
  inc.update(data, data_sz);
  inc.update((char *)&partition, sizeof(partition));
  return inc.digest();
}

wal_write_reply::iterator
wal_write_reply::find(const seastar::sstring &topic, uint32_t partition) {
  return cache_.find(idx(topic.data(), topic.size(), partition));
}
wal_write_reply::iterator
wal_write_reply::find(const char *data, uint32_t partition) {
  return cache_.find(idx(data, std::strlen(data), partition));
}
wal_write_reply::iterator
wal_write_reply::find(const char *       data,
                      const std::size_t &size,
                      uint32_t           partition) {
  return cache_.find(idx(data, size, partition));
}

}  // namespace smf
