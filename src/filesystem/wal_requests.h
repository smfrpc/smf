// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <list>
#include <numeric>
#include <utility>

#include <core/fair_queue.hh>
#include <core/file.hh>
#include <core/shared_ptr.hh>
#include <core/sstring.hh>

#include "adt/flat_hash_map.h"
#include "flatbuffers/wal_generated.h"
#include "platform/log.h"
#include "platform/macros.h"
#include "seastar_io/priority_manager.h"

// class seastar::io_priority_class;

// Note that the put/get requests are pointers because they typically come from
// an RPC call. it is the responsibility of the caller to ensure that the
// requests and pointers stay valid throughout the request lifecycle.
//
// In addition, they must be cheap to copy-construct - typically a couple of
// pointer copy.
//

namespace smf {
static constexpr std::size_t kWalHeaderSize = sizeof(wal::wal_header);
static constexpr std::size_t kOnDiskInternalHeaderSize = 9;

namespace details {
template <typename T>
struct priority_wrapper {
  priority_wrapper(const T *ptr, const ::seastar::io_priority_class &p)
    : req(THROW_IFNULL(ptr)), pc(p) {}
  priority_wrapper(priority_wrapper &&o) noexcept
    : req(std::move(o.req)), pc(std::move(o.pc)) {}
  priority_wrapper(const priority_wrapper &o) : req(o.req), pc(o.pc) {}
  const T *                           req;
  const ::seastar::io_priority_class &pc;
};
}  // namespace details
}  // namespace smf

namespace smf {


// reads

struct wal_read_request : details::priority_wrapper<smf::wal::tx_get_request> {
  wal_read_request(const smf::wal::tx_get_request *    ptr,
                   const ::seastar::io_priority_class &p)
    : priority_wrapper(ptr, p) {}


  static bool validate(const wal_read_request &r);
};

class wal_read_reply {
 public:
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_read_reply);

  explicit wal_read_reply(uint64_t request_offset);
  inline void
  reset_consume_offset() {
    consume_offset_ = 0;
  }
  inline uint64_t
  get_consume_offset() const {
    return consume_offset_;
  }
  inline void
  update_consume_offset(uint64_t next) {
    data_->next_offset = (data_->next_offset - consume_offset_) + next;
    consume_offset_    = next;
  }
  inline wal::tx_get_replyT *
  reply() const {
    return data_.get();
  }
  inline uint64_t
  next_epoch() const {
    return data_->next_offset;
  }
  uint64_t on_disk_size();
  uint64_t on_wire_size();

  inline bool
  empty() const {
    return data_->gets.empty();
  }

 private:
  std::unique_ptr<wal::tx_get_replyT> data_ =
    std::make_unique<wal::tx_get_replyT>();
  uint64_t consume_offset_ = {0};
};

// writes

struct wal_write_request : details::priority_wrapper<smf::wal::tx_put_request> {
  using fb_const_iter_t = typename flatbuffers::Vector<
    flatbuffers::Offset<smf::wal::tx_put_partition_tuple>>::const_iterator;

  class write_request_iterator
    : std::iterator<std::input_iterator_tag, fb_const_iter_t> {
   public:
    using const_iter_t = fb_const_iter_t;
    write_request_iterator(const std::set<uint32_t> &view,
                           const_iter_t              start,
                           const_iter_t              end);

    write_request_iterator(const write_request_iterator &o);
    write_request_iterator &next();
    write_request_iterator &operator++();
    write_request_iterator  operator++(int);
    const_iter_t            operator->();
    const_iter_t            operator*();
    bool                    operator==(const write_request_iterator &o);
    bool                    operator!=(const write_request_iterator &o);

   private:
    const std::set<uint32_t> &view_;
    const_iter_t              start_;
    const_iter_t              end_;
  };

  wal_write_request(const smf::wal::tx_put_request *    ptr,
                    const ::seastar::io_priority_class &p,
                    const uint32_t                      assigned_core,
                    const uint32_t                      runner_core,
                    const std::set<uint32_t> &          partitions);

  wal_write_request(const wal_write_request &) = default;
  wal_write_request &operator=(const wal_write_request &) = default;

  write_request_iterator begin();
  write_request_iterator end();

  const uint32_t           assigned_core;
  const uint32_t           runner_core;
  const std::set<uint32_t> partition_view;

  static bool validate(const wal_write_request &r);
};


/// \brief exposes a nested hashing for the underlying map
class wal_write_reply {
 public:
  using underlying =
    flat_hash_map<uint64_t,
                  std::unique_ptr<wal::tx_put_reply_partition_tupleT>>;
  using iterator       = typename underlying::iterator;
  using const_iterator = typename underlying::const_iterator;

  SMF_DISALLOW_COPY_AND_ASSIGN(wal_write_reply);
  wal_write_reply() {}
  wal_write_reply(wal_write_reply &&o) noexcept : cache_(std::move(o.cache_)) {}

  void     set_reply_partition_tuple(const seastar::sstring &topic,
                                     uint32_t                partition,
                                     uint64_t                begin,
                                     uint64_t                end);
  iterator find(const seastar::sstring &topic, uint32_t partition);
  iterator find(const char *data, uint32_t partition);
  iterator find(const char *data, const std::size_t &size, uint32_t partition);

  /// \brief drains the internal cache and moves all elements into this
  std::unique_ptr<smf::wal::tx_put_replyT> move_into();

  iterator
  begin() {
    return cache_.begin();
  }
  const_iterator
  begin() const {
    return cache_.begin();
  }
  const_iterator
  cbegin() const {
    return begin();
  }
  iterator
  end() {
    return cache_.end();
  }
  const_iterator
  end() const {
    return cache_.end();
  }
  const_iterator
  cend() const {
    return end();
  }
  bool
  empty() const {
    return cache_.empty();
  }
  size_t
  size() const {
    return cache_.size();
  }

 private:
  uint64_t idx(const char *data, std::size_t data_sz, uint32_t partition);

 private:
  flat_hash_map<uint64_t, std::unique_ptr<wal::tx_put_reply_partition_tupleT>>
    cache_{};
};

}  // namespace smf
