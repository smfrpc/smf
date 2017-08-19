// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <list>
#include <numeric>
#include <utility>

#include <core/fair_queue.hh>
#include <core/file.hh>

#include "flatbuffers/wal_generated.h"
#include "platform/log.h"
#include "platform/macros.h"

// class seastar::io_priority_class;

// Note that the put/get requests are pointers because they typically come from
// an RPC call. it is the responsibility of the caller to ensure that the
// requests
// and pointers stay valid throughout the request lifecycle.

namespace smf {
enum class wal_type : uint8_t {
  wal_type_disk_with_memory_cache,
  // for testing only
  wal_type_memory_only
};

// reads

struct wal_read_request {
  wal_read_request(smf::wal::tx_get_request *    ptr,
                   ::seastar::io_priority_class &p)
    : req(THROW_IFNULL(ptr)), pc(p) {}
  wal_read_request(wal_read_request &&o) noexcept
    : req(std::move(o.req)), pc(std::move(o.pc)) {}

  smf::wal::tx_get_request *          req;
  const ::seastar::io_priority_class &pc;
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
    seastar::lw_make_shared<wal::tx_get_replyT>();
};

// writes

struct wal_write_request {
  wal_write_request(smf::wal::tx_put_request *          ptr,
                    const ::seastar::io_priority_class &p)
    : req(THROW_IFNULL(ptr)), pc(p) {}

  wal_write_request(wal_write_request &&o) noexcept
    : req(std::move(o.ptr)), pc(o.pc) {}

  smf::wal::tx_put_request *          ptr;
  const ::seastar::io_priority_class &pc;
};
using wal_write_reply = smf::wal::tx_put_reply;


// invalidations

struct wal_write_invalidation {
  wal_write_request(smf::wal::tx_put_invalidation *     ptr,
                    const ::seastar::io_priority_class &p)
    : req(THROW_IFNULL(ptr)), pc(p) {}

  wal_write_request(wal_write_request &&o) noexcept
    : req(std::move(o.ptr)), pc(o.pc) {}

  smf::wal::tx_put_invalidation *     ptr;
  const ::seastar::io_priority_class &pc;
}
}  // namespace smf
