#pragma once

#include "smf/rpc_filter.h"

namespace smf {

class rpc_envelope;
class rpc_recv_context;

struct failure_injector_recv_filter : rpc_filter<rpc_envelope> {
  seastar::future<rpc_recv_context> operator()(rpc_recv_context &&ctx);
};

}  // namespace smf
