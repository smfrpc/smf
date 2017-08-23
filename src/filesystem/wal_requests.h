// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <list>
#include <numeric>
#include <utility>

#include <core/fair_queue.hh>
#include <core/file.hh>
#include <core/shared_ptr.hh>

#include "flatbuffers/wal_generated.h"
#include "platform/log.h"
#include "platform/macros.h"

// class seastar::io_priority_class;

// Note that the put/get requests are pointers because they typically come from
// an RPC call. it is the responsibility of the caller to ensure that the
// requests and pointers stay valid throughout the request lifecycle.
//
// In addition, they must be cheap to copy-construct - typically a couple of
// pointer copy.
//

namespace smf {
namespace details {
template <typename T> struct priority_wrapper {
  priority_wrapper(T *ptr, const ::seastar::io_priority_class &p)
    : req(THROW_IFNULL(ptr)), pc(p) {}
  priority_wrapper(priority_wrapper &&o) noexcept
    : req(std::move(o.req)), pc(std::move(o.pc)) {}
  T *                                 req;
  const ::seastar::io_priority_class &pc;
};
}  // namespace details
}  // namespace smf

namespace smf {
enum class wal_type : uint8_t {
  wal_type_disk_with_memory_cache,
  // for testing only
  wal_type_memory_only
};


// reads

struct wal_read_request : details::priority_wrapper<smf::wal::tx_get_request> {
  wal_read_request(smf::wal::tx_get_request *    ptr,
                   ::seastar::io_priority_class &p)
    : priority_wrapper(ptr, p) {}
};
struct wal_read_reply {
  wal_read_reply() {}
  wal_read_reply(wal_read_reply &&r) noexcept : data(std::move(r.data)) {}

  // This needs a unit tests and integration tests
  // TODO(agallego) -
  // The reason is that every time we modify the wal.fbs schema we have to add
  // sizeof(structs) for all structs +
  // add all dynamic sizes
  //
  uint64_t size() {
    return std::accumulate(data->gets.begin(), data->gets.end(), uint64_t(0),
                           [](uint64_t acc, const auto &it) {
                             return acc + it.compresed_txns.size()
                                    + sizeof(wal::wal_header);
                           });
  }

  inline bool empty() const { return data->gets.empty(); }

  seastar::lw_shared_ptr<wal::tx_get_replyT> data =
    seastar::make_lw_shared<wal::tx_get_replyT>();
};

// writes

struct wal_write_request : details::priority_wrapper<smf::wal::tx_put_request> {
  class write_request_iterator
    : std::iterator<std::input_iterator_tag, wal::tx_put_partition_pair> {
   public:
    using iter_t =
      typename flatbuffers::Vector<wal::tx_put_partition_pair>::iterator;

    write_request_iterator(const std::set<uint32_t> &view,
                           iter_t                    start,
                           iter_t                    end)
      : view_(view), start_(start), end_(end) {}
    void operator++() {
      if (start_ != end_) start_++;
    }
    iter_t operator*() { return start_; }
    bool operator==(const write_request_iterator &o) {
      return start_ == o.start_ && view_ == o.view_ && end_ == o.end_;
    }
    bool operator!=(const write_request_iterator &o) { return !(*this == o); }

   private:
    const std::set<uint32_t> &view_;
    iter_t                    start_;
    iter_t                    end_;
  };
  wal_write_request(smf::wal::tx_put_request *          ptr,
                    const ::seastar::io_priority_class &p,
                    const std::set<uint32_t> &          partitions)
    : priority_wrapper(ptr, p), partition_view(partitions) {}
  write_request_iterator begin() {
    return write_request_iterator(partition_view, req->data->begin(),
                                  req->data->end());
  }
  write_request_iterator end() {
    return write_request_iterator(partition_view, req->data->end(),
                                  req->data->end());
  }
  const std::set<uint32_t> partition_view;
};

struct wal_write_reply {
  wal_write_reply(uint64_t offset) { data->offset = offset; }
  seastar::lw_shared_ptr<smf::wal::tx_put_replyT> data =
    seastar::lw_make_shared<wal::tx_put_replyT>();
};


// invalidations

struct wal_write_invalidation
  : details::priority_wrapper<smf::wal::tx_put_invalidation> {
  wal_write_invalidation(smf::wal::tx_put_invalidation *     ptr,
                         const ::seastar::io_priority_class &p)
    : priority_wrapper(ptr, p) {}
};

}  // namespace smf
