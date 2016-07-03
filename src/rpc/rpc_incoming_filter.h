#pragma once
#include "rpc/rpc_recv_context.h"
namespace smf {
  struct rpc_incoming_filter {
    future<rpc_recv_context> apply(rpc_recv_context &&ctx) = 0
  };
}
