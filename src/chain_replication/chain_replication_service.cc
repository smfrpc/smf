// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "chain_replication/chain_replication_service.h"

#include <utility>

#include "chain_replication/chain_replication_utils.h"
#include "filesystem/wal_requests.h"
#include "hashing/hashing_utils.h"
#include "seastar_io/priority_manager.h"

namespace smf {
namespace chains {
future<smf::rpc_envelope> chain_replication_service::put(
  smf::rpc_recv_typed_context<tx_put_request> &&record) {
  unsigned core = put_to_lcore(record.get());
  return smp::submit_to(core, [this, record = std::move(record)]() mutable {
           // do the actual put on the correct core
           return do_put(std::move(record)).then([](auto ctx) {
             auto ctx_p = make_lw_shared<smf::rpc_envelope>(std::move(ctx));
             return make_ready_future<lw_shared_ptr<smf::rpc_envelope>>(ctx_p);
           });
         })
    .then([](auto ctx_ptr) {
      return make_ready_future<smf::rpc_envelope>(std::move(*ctx_ptr));
    });
}

future<smf::rpc_envelope> chain_replication_service::do_put(
  smf::rpc_recv_typed_context<tx_put_request> &&put) {
  auto body = put.ctx.payload()->body();

  temporary_buffer<char> buf(body->size());
  std::copy(body->begin(), body->end(), buf.get_write());

  smf::wal_write_request r(
    0, std::move(buf),
    smf::priority_manager::thread_local_instance().streaming_write_priority());

  return wal_->local()
    .append(std::move(r))
    .then([](uint64_t last_index) {
      smf::rpc_envelope e;
      e.set_status(200);
      return make_ready_future<smf::rpc_envelope>(std::move(e));
    })
    .handle_exception([](std::exception_ptr eptr) {
      LOG_ERROR("Error saving smf::chains::sput()");
      smf::rpc_envelope e;
      e.set_status(500);
      return make_ready_future<smf::rpc_envelope>(std::move(e));
    });
}

future<smf::rpc_envelope> chain_replication_service::get(
  smf::rpc_recv_typed_context<tx_get_request> &&record) {
  smf::rpc_envelope e;
  e.set_status(501);  // Not implemented
  return make_ready_future<smf::rpc_envelope>(std::move(e));
}
}  // end namespace chains
}  // end namespace smf
