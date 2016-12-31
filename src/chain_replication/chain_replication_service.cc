#include "chain_replication/chain_replication_service.h"
#include "chain_replication/chain_replication_utils.h"
#include "hashing_utils.h"
namespace smf {
namespace chains {
future<smf::rpc_envelope> chain_replication_service::mput(
  smf::rpc_recv_typed_context<tx_multiput> &&puts) {
  smf::rpc_envelope e(nullptr);
  e.set_status(501); // Not implemented
  return make_ready_future<smf::rpc_envelope>(std::move(e));
}

future<smf::rpc_envelope>
chain_replication_service::sput(smf::rpc_recv_typed_context<tx_put> &&put) {
  if(!put) {
    smf::rpc_envelope e(nullptr);
    e.set_status(501); // Not implemented
    return make_ready_future<smf::rpc_envelope>(std::move(e));
  }
  auto core_to_handle = put_to_lcore(put.get());
  return smp::submit_to(core_to_handle,
                        [p = std::move(put)]{

                        })
    // .handle_exception([](std::exception_ptr eptr) {
    //   LOG_ERROR("Error saving smf::chains::sput()");
    //   smf::rpc_envelope e(nullptr);
    //   e.set_status(500);
    //   return make_ready_future<smf::rpc_envelope>(std::move(e));
    // })
    .then([] {
      smf::rpc_envelope e(nullptr);
      e.set_status(200);
      return make_ready_future<smf::rpc_envelope>(std::move(e));
    });
}

future<smf::rpc_envelope> chain_replication_service::fetch(
  smf::rpc_recv_typed_context<tx_fetch_request> &&rec) {
  smf::rpc_envelope e(nullptr);
  e.set_status(501); // Not implemented
  return make_ready_future<smf::rpc_envelope>(std::move(e));
}
} // end namespace chains
} // end namespace smf
