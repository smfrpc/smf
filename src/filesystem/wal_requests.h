// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <list>
#include <numeric>
#include <utility>
// third party
#include <core/fair_queue.hh>
#include <core/file.hh>
// smf
#include "flatbuffers/wal_generated.h"
#include "platform/log.h"
#include "platform/macros.h"

// class seastar::io_priority_class;

namespace smf {
enum class wal_type : uint8_t {
  wal_type_disk_with_memory_cache,
  wal_type_memory_only
};

struct wal_read_request {
  using get_t     = smf::wal::tx_get_request;
  using get_ptr_t = seastar::lw_shared_ptr<rpc_recv_typed_context<get_t>>;

  wal_read_request(get_ptr_t ptr, ::seastar::io_priority_class &p)
    : req(ptr), pc(p) {}
  wal_read_request(wal_read_request &&o) noexcept
    : req(std::move(o.req)), pc(std::move(o.pc)) {}


  // need to use a shared_ptr to share around the multiple wal IO stuff
  get_ptr_t                           req;
  const ::seastar::io_priority_class &pc;
};


struct wal_read_reply {
  wal_read_reply() {}

  wal_read_reply(wal_read_reply &&r) noexcept : data(std::move(r.data)) {}

  uint64_t size() {
    return std::accumulate(data->gets.begin(), data->gets.end(), uint64_t(0),
                           [](uint64_t acc, const auto &it) {
                             return acc + it.compresed_txns.size()
                                    + sizeof(wal::wal_header);
                           });
  }
  inline bool empty() const { return data->gets.empty(); }

  auto data = seastar::lw_make_shared<wal::tx_get_reply>();
};

struct wal_write_request {
  wal_write_request(uint32_t                            _flags,
                    seastar::temporary_buffer<char> &&  _data,
                    const ::seastar::io_priority_class &_pc)
    : flags(_flags), data(std::move(_data)), pc(_pc) {}

  wal_write_request(wal_write_request &&w) noexcept
    : flags(w.flags), data(std::move(w.data)), pc(w.pc) {}

  wal_write_request share_range(size_t i, size_t len) {
    LOG_THROW_IF(i + len > data.size(), "Out of bounds error");
    return wal_write_request(flags, data.share(i, len), pc);
  }


  uint32_t                        flags;
  seastar::temporary_buffer<char> data;

  const ::seastar::io_priority_class &pc;
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_write_request);
};
}  // namespace smf
