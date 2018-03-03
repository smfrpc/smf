// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "smf/rpc_handle_router.h"


namespace smf {

std::ostream &
operator<<(std::ostream &o, const smf::rpc_handle_router &r) {
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
smf::rpc_service_method_handle *
rpc_handle_router::get_handle_for_request(const uint32_t &request_id) {
  for (auto &p : services_) {
    auto x = p->method_for_request_id(request_id);
    if (x != nullptr) return x;
  }
  return nullptr;
}

void
rpc_handle_router::register_service(std::unique_ptr<rpc_service> s) {
  assert(s != nullptr);
  services_.push_back(std::move(s));
}
}  // namespace smf
