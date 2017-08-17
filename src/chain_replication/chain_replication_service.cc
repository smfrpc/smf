// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "chain_replication/chain_replication_service.h"

#include <utility>

#include "chain_replication/chain_replication_utils.h"
#include "filesystem/wal_requests.h"
#include "hashing/hashing_utils.h"
#include "rpc/futurize_utils.h"
#include "seastar_io/priority_manager.h"

namespace smf {
namespace chains {
using namespace smf;
using namespace smf::wal;

seastar::future<rpc_typed_envelope<tx_put_reply>>
chain_replication_service::put(
  rpc_recv_typed_context<tx_put_request> &&record) {
  if (!record) {
    return futurize_status_for_type<tx_put_reply>(400);
  }
  auto core_to_handle = put_to_lcore(record.get());
  return seastar::smp::submit_to(
           core_to_handle, [this, p = std::move(record)]() mutable {
             auto body = p.ctx.value().payload->mutable_body();
             seastar::temporary_buffer<char> buf(body->size());

             auto src = reinterpret_cast<const char *>(body->data());
             auto end = reinterpret_cast<const char *>(src + body->size());

             std::copy(src, end, buf.get_write());
             auto &write_priority = priority_manager::thread_local_instance()
                                      .streaming_write_priority();
             wal_write_request r(0, std::move(buf), write_priority);

             return wal_->local().append(std::move(r));
           })
    .then([](uint64_t last_index) {
      rpc_typed_envelope<tx_put_reply> data;
      data.data->set_offset(last_index);
      data.envelope.set_status(200);
      return seastar::make_ready_future<rpc_typed_envelope<tx_put_reply>>(
        std::move(data));
    })
    .handle_exception([](auto eptr) {
      LOG_ERROR("Error saving chains::put(): {}", eptr);
      return futurize_status_for_type<tx_put_reply>(501);
    });
}


seastar::future<rpc_typed_envelope<tx_get_reply>>
chain_replication_service::get(
  rpc_recv_typed_context<tx_get_request> &&record) {
  rpc_typed_envelope<tx_get_reply> data;

  if (!record) {
    return futurize_status_for_type<tx_get_reply>(400);
  }

  auto core_to_handle = get_to_lcore(record.get());
  return seastar::smp::submit_to(
           core_to_handle, [this, r = std::move(record)]() mutable {
             // TODO(agallego) - incorporate topics
             // across the wal's and here as well
             auto &read_priority = priority_manager::thread_local_instance()
                                     .streaming_read_priority();
             wal_read_request read(r->offset(), r->max_bytes(), 0,
                                   read_priority);
             return wal_->local()
               .get(std::move(read))
               .then([old_offset = r->offset()](auto maybe_read) {
                 rpc_typed_envelope<tx_get_reply> reply;
                 reply.envelope.set_status(200);

                 if (!maybe_read || maybe_read.empty()) {
                   reply.data->set_next_offset(old_offset);
                 } else {
                   // update offset to max & set fragments here
                   auto read_data = std::move(maybe_read.value());
                   reply.data->set_next_offset(old_offset + read_data.size());

                   for (auto it  = read_data.fragments.begin(),
                             end = read_data.fragments.end();
                        it != end; ++it) {
                     const auto flags = it->hdr.wal_entry_flags();
                     if (flags & fbs::wal::wal_entry_flags::
                                   wal_entry_flags_partial_fagment) {
                     }


                     // THE PLAN
                     // 1) reconstruct the original payload
                     // 2) inflate the zstd buffer
                     // 3) return the txns - already formatted correctly on
                     // input :)
                     // 4) ensure that we do this PER full tx_op otherwise we
                     // have to read
                     //    more data until we hit this limit no?
                     // return the max bytes... and go home.


                     if (flags & fbs::wal::wal_entry_flags::
                                   wal_entry_flags_full_fagment) {
                     }
                     // convert wal::fragment -> chain::fragment
                     // TODO(agallego) - make this a util and add a unit tests
                     //
                     auto frag = std::make_unique<chains::tx_fragmentT>();
                     frag->op  = chains::tx_operation::tx_operation_full;
                     frag->id  = static_cast<uint32_t>(random().next());
                     frag->time_micros = time_now_micros();

                     frag->key.reserve(key.size());
                     frag->value.reserve(value.size());
                   }
                 }
                 return seastar::make_ready_future<decltype(reply)>(
                   std::move(reply));
               });
           })
    .handle_exception([](auto eptr) {
      LOG_ERROR("Error getting data chains::get(): {}", eptr);
      return futurize_status_for_type<tx_get_reply>(501);
    });
}
}  // end namespace chains
}  // end namespace smf
