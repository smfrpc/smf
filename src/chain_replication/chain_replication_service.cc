// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "chain_replication/chain_replication_service.h"
#include <utility>
#include "chain_replication/chain_replication_utils.h"
#include "hashing/hashing_utils.h"

namespace smf {
namespace chains {
future<smf::rpc_envelope> chain_replication_service::put(
  smf::rpc_recv_typed_context<tx_put_request> &&record) {
  if (!record) {
    smf::rpc_envelope e(nullptr);
    e.set_status(501);  // Not implemented
    return make_ready_future<smf::rpc_envelope>(std::move(e));
  }
  auto core_to_handle = put_to_lcore(record.get());
  return smp::submit_to(core_to_handle, [p = std::move(record)]{})
    // .handle_exception([](std::exception_ptr eptr) {
    //   LOG_ERROR("Error saving smf::chains::sput()");
    //   smf::rpc_envelope e(nullptr);
    //   e.set_status(500);
    // })
    .then([] {
      smf::rpc_envelope e(nullptr);
      e.set_status(200);
      return make_ready_future<smf::rpc_envelope>(std::move(e));
    });
}

future<smf::rpc_envelope> chain_replication_service::get(
  smf::rpc_recv_typed_context<tx_get_request> &&record) {
  smf::rpc_envelope e(nullptr);
  e.set_status(501);  // Not implemented
  return make_ready_future<smf::rpc_envelope>(std::move(e));
}
}  // end namespace chains
}  // end namespace smf
