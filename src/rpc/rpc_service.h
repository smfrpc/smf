#pragma once
#include "rpc/rpc_recv_context.h"
#include "rpc/rpc_envelope.h"
namespace smf {
// https://github.com/grpc/grpc/blob/d0fbba52d6e379b76a69016bc264b96a2318315f/include/grpc%2B%2B/impl/codegen/rpc_method.h
struct rpc_service_method_handle {
  // TODO(agallego) - expose through generator
  // now it  the default is server_streaming
  enum rpc_type {
    NORMAL_RPC = 0,
    CLIENT_STREAMING, // request streaming
    SERVER_STREAMING, // response streaming
    BIDI_STREAMING
  };

  using fn_t = std::function<future<rpc_envelope>(rpc_recv_context &&recv)>;
  rpc_service_method_handle(const char *name, const uint32_t &id, fn_t f)
    : method_name(name), method_id(id), apply(f) {}

  rpc_service_method_handle(const rpc_service_method_handle &o)
    : method_name(o.method_name), method_id(o.method_id), apply(o.apply) {}

  const char *method_name;
  const uint32_t method_id;
  fn_t apply;
};

struct rpc_service {
  virtual const char *service_name() = 0;
  /// \brief crc32 of service_name or CityHash32(service_name())
  virtual uint32_t service_id() = 0;
  virtual std::vector<rpc_service_method_handle> methods() = 0;
};
}
