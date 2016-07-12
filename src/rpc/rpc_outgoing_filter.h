#pragma once
#include "rpc/rpc_recv_context.h"
namespace smf {
struct rpc_outgoing_filter {
  virtual future<rpc_envelope> apply(rpc_envelope &&e) = 0;
};
}
