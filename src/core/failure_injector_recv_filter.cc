#include "smf/failure_injector_recv_filter.h"

#include "smf/log.h"
#include "smf/rpc_envelope.h"
#include "smf/rpc_recv_context.h"

namespace smf {
seastar::future<rpc_recv_context>
failure_injector_recv_filter::operator()(rpc_recv_context &&ctx) {
  if (SMF_LIKELY(ctx)) {
    if (ctx.header.biflags &
        header_bitflags::header_bitflags_has_dynamic_headers) {
      // TODO(agallego) impl ldfi_fspec
    }
  }
  return seastar::make_ready_future<rpc_recv_context>(std::move(ctx));
}

}  // namespace smf
