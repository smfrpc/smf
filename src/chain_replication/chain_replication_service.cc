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

seastar::future<smf::rpc_typed_envelope<tx_put_reply>>
chain_replication_service::put(
  smf::rpc_recv_typed_context<tx_put_request> &&record) {
  if (!record) {
    return smf::futurize_status_for_type<tx_put_reply>(400);
  }
  auto core_to_handle = put_to_lcore(record.get());
  return seastar::smp::submit_to(
           core_to_handle, [this, p = std::move(record)]() mutable {
             auto body = p.ctx.value().payload->mutable_body();
             seastar::temporary_buffer<char> buf(body->size());

             auto src = reinterpret_cast<const char *>(body->data());
             auto end = reinterpret_cast<const char *>(src + body->size());

             std::copy(src, end, buf.get_write());

             smf::wal_write_request r(
               0, std::move(buf), smf::priority_manager::thread_local_instance()
                                    .streaming_write_priority());

             return wal_->local().append(std::move(r));
           })
    .then([](uint64_t last_index) {
      smf::rpc_typed_envelope<tx_put_reply> data;
      data.envelope.set_status(200);
      return seastar::make_ready_future<smf::rpc_typed_envelope<tx_put_reply>>(
        std::move(data));
    })
    .handle_exception([](auto eptr) {
      LOG_ERROR("Error saving smf::chains::sput(): {}", eptr);
      return smf::futurize_status_for_type<tx_put_reply>(501);
    });
}


seastar::future<smf::rpc_typed_envelope<tx_get_reply>>
chain_replication_service::get(
  smf::rpc_recv_typed_context<tx_get_request> &&record) {
  smf::rpc_typed_envelope<tx_get_reply> data;

  if (!record) {
    return smf::futurize_status_for_type<tx_get_reply>(400);
  }

  // auto core_to_handle = get_to_lcore(record.get());
  return smf::futurize_status_for_type<tx_get_reply>(501);
}
}  // end namespace chains
}  // end namespace smf
