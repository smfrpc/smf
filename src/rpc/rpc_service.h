#pragma once
#include "rpc/rpc_recv_context.h"
#include "rpc/rpc_envelope.h"
namespace smf {

struct rpc_service_method_handle {
  virtual const char *method_name() = 0;
  virtual uint32_t method_id() = 0;
  virtual future<rpc_envelope> apply(rpc_recv_context &&recv) = 0;
};

struct rpc_service {
  virtual const char *service_name() = 0;
  /// \brief crc32 of service_name or CityHash32(service_name())
  virtual uint32_t service_id() = 0;
  virtual std::unordered_map<uint32_t, rpc_service_method_handle *>
  methods() = 0;
};
}
