#include "rpc/rpc_handle_router.h"

namespace smf {
 bool rpc_handle_router::can_handle_request(
    const uint32_t &request_id,
    const flatbuffers::Vector<flatbuffers::Offset<fbs::rpc::DynamicHeader>>
      *hdrs) {
    if(dispatch_.find(request_id) == dispatch_.end()) {
      return false;
    }
    return true;
  }
 future<rpc_envelope> rpc_handle_router::handle(rpc_recv_context &&recv) {
    return dispatch_[recv.request_id()]->apply(std::move(recv));
  }

  void rpc_handle_router::register_rpc_service(rpc_service *s) {
    assert(s != nullptr);
    uint32_t srv = s->service_id();
    for(auto t : s->methods()) {
      // apply xor hash
      auto id = (srv ^ t.first);
      dispatch_.insert({id, t.second});
    }
  }
} // namespace
