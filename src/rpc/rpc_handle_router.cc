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
  return dispatch_.find(recv.request_id())->second.apply(std::move(recv));
}

void rpc_handle_router::register_service(std::unique_ptr<rpc_service> s) {
  assert(s != nullptr);
  uint32_t srv = s->service_id();
  for(auto h : s->methods()) {
    // apply xor hash
    auto id = (srv ^ h.method_id);
    dispatch_.insert({id, h});
  }
  services_.push_back(std::move(s));
}
} // namespace
