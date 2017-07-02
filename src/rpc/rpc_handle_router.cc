// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_handle_router.h"


namespace smf {

std::ostream &operator<<(std::ostream &o, const smf::rpc_handle_router &r) {
  o << "rpc_handle_router{";
  for (const auto &service : r.services_) {
    o << "rpc_service{name=" << service->service_name() << ",handles=";
    for (const auto &method : service->methods()) {
      o << "rpc_service_method_handle{name=" << method.method_name << "}";
    }
    o << "}";
  }
  o << "}";
  return o;
}

bool rpc_handle_router::can_handle_request(const uint32_t &request_id) {
  return dispatch_.find(request_id) != dispatch_.end();
}

seastar::future<rpc_envelope> rpc_handle_router::handle(
  rpc_recv_context &&recv) {
  return dispatch_.find(recv.request_id())->second.apply(std::move(recv));
}

void rpc_handle_router::register_service(std::unique_ptr<rpc_service> s) {
  assert(s != nullptr);
  uint32_t srv = s->service_id();
  for (auto h : s->methods()) {
    // apply xor hash
    auto id = (srv ^ h.method_id);
    dispatch_.insert({id, h});
  }
  services_.push_back(std::move(s));
}
}  // namespace smf
