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

wal_write_request::write_request_iterator::write_request_iterator(
  const std::set<uint32_t> &view, const_iter_t start, const_iter_t end)
  : view_(view), start_(start), end_(end) {
  if (start_ != end_ && view_.find(start_->partition()) == view_.end()) {
    next();
  }
}

wal_write_request::write_request_iterator::write_request_iterator(
  const write_request_iterator &o)
  : view_(o.view_), start_(o.start_), end_(o.end_) {
  if (start_ != end_ && view_.find(start_->partition()) == view_.end()) {
    next();
  }
}

wal_write_request::write_request_iterator &
wal_write_request::write_request_iterator::next() {
  while (start_ != end_) {
    ++start_;
    if (start_ != end_ && view_.find(start_->partition()) != view_.end()) {
      break;
    }
  }
  return *this;
}

wal_write_request::write_request_iterator
  &wal_write_request::write_request_iterator::operator++() {
  return next();
}
wal_write_request::write_request_iterator
  wal_write_request::write_request_iterator::operator++(int) {
  write_request_iterator retval = *this;
  next();
  return retval;
}
wal_write_request::write_request_iterator::const_iter_t
  wal_write_request::write_request_iterator::operator->() {
  return start_;
}
wal_write_request::write_request_iterator::const_iter_t
  wal_write_request::write_request_iterator::operator*() {
  return start_;
}
bool
wal_write_request::write_request_iterator::operator==(
  const wal_write_request::write_request_iterator &o) {
  return start_ == o.start_ && end_ == o.end_ && view_ == o.view_;
}
bool
wal_write_request::write_request_iterator::operator!=(
  const wal_write_request::write_request_iterator &o) {
  return !(*this == o);
}


wal_write_request::wal_write_request(const smf::wal::tx_put_request *    ptr,
                                     const ::seastar::io_priority_class &p,
                                     const uint32_t            _assigned_core,
                                     const uint32_t            _runner_core,
                                     const std::set<uint32_t> &partitions)
  : priority_wrapper(ptr, p)
  , assigned_core(_assigned_core)
  , runner_core(_runner_core)
  , partition_view(partitions) {
  DLOG_THROW_IF(partition_view.empty(), "Partition view is empty");
}

wal_write_request::write_request_iterator
wal_write_request::begin() {
  return write_request_iterator(partition_view, req->data()->begin(),
                                req->data()->end());
}
wal_write_request::write_request_iterator
wal_write_request::end() {
  return write_request_iterator(partition_view, req->data()->end(),
                                req->data()->end());
}
bool
wal_write_request::valid(const wal_write_request &r) {
  DLOG_INFO_IF(r.partition_view.empty(), "Partition view is empty!");
  try {
    // traversing the request object might throw
    (void)r.req->topic();
    (void)r.req->data();
    return !r.partition_view.empty() && r.req->topic()->size() != 0;
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
