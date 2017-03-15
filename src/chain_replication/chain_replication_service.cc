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
  auto core_to_handle = put_to_lcore(record.get());
  return smp::submit_to(
    core_to_handle,
    [this, p = std::move(record)]() mutable { return do_put(std::move(p)); });
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
