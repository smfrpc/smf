// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <seastar/util/noncopyable_function.hh>

#include "smf/rpc_envelope.h"
#include "smf/rpc_recv_context.h"

namespace smf {
// https://github.com/grpc/grpc/blob/d0fbba52d6e379b76a69016bc264b96a2318315f/include/grpc%2B%2B/impl/codegen/rpc_method.h
struct rpc_service_method_handle {
  // TODO(agallego) - expose through generator
  // now it  the default is server_streaming
  enum rpc_type {
    NORMAL_RPC = 0,
    CLIENT_STREAMING,  // request streaming
    SERVER_STREAMING,  // response streaming
    BIDI_STREAMING
  };

  using fn_t = seastar::noncopyable_function<seastar::future<rpc_envelope>(
    rpc_recv_context &&recv)>;

  rpc_service_method_handle(fn_t &&f) : apply(std::move(f)) {}
  ~rpc_service_method_handle() = default;

  fn_t apply;
};

struct rpc_service {
  virtual const char *service_name() const = 0;
  virtual uint32_t service_id() const = 0;
  virtual rpc_service_method_handle *method_for_request_id(uint32_t idx) = 0;
  virtual std::ostream &print(std::ostream &) const = 0;
  virtual ~rpc_service() {}
  rpc_service() {}
};
}  // namespace smf

// namespace std {
// ostream &operator<<(ostream &o, const smf::rpc_service *s);
// }  // namespace std
