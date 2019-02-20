#include "smf/failure_injector_recv_filter.h"

#include "smf/log.h"
#include "smf/rpc_envelope.h"
#include "smf/rpc_recv_context.h"

namespace smf {
seastar::future<rpc_recv_context>
fialure_injector_recv_filter::operator()(rpc_recv_context &&ctx) {
  // TODO(agallego) parse header and throw or sleep
  return seastar::make_ready_future<rpc_recv_context>(std::move(ctx));
}

}  // namespace smf
