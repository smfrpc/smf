#pragma once

#include "smf/rpc_filter.h"
#include "smf/rpc_generated.h"
#include "smf/stdx.h"

namespace smf {

class rpc_envelope;
class rpc_recv_context;

struct failure_injector_recv_filter : rpc_filter<rpc_envelope> {
  seastar::future<rpc_recv_context> operator()(rpc_recv_context &&ctx);

  // for tests
  static stdx::optional<rpc::failure_spec>
  find_failure_spec(const rpc::dynamic_headers *hdrs);
};

}  // namespace smf
