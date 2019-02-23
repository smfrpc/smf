#include "smf/failure_injector_recv_filter.h"

#include <chrono>
#include <cstring>
#include <exception>  // std::terminate

#include <seastar/core/sleep.hh>

#include "smf/log.h"
#include "smf/rpc_envelope.h"
#include "smf/rpc_recv_context.h"
#include "smf/stdx.h"

namespace smf {
class failure_injector_recv_filter_exception final : public std::exception {
 public:
  virtual const char *
  what() const noexcept {
    return "Injected by `failure_injector_recv_filter`";
  }
};

stdx::optional<rpc::failure_spec>
find_failure_spec(const rpc::dynamic_headers *hdrs) {
  rpc::failure_spec spec;
  for (const rpc::header_kv *kv : *(hdrs->values())) {
    if (kv->KeyCompareWithValue("x-smf-failure-spec")) {
      std::memcpy(&spec, kv->hdr_value()->data(), kv->hdr_value()->size());
      return spec;
    }
  }
  return stdx::nullopt;
}

seastar::future<rpc_recv_context>
failure_injector_recv_filter::operator()(rpc_recv_context &&ctx) {
  if (ctx.header.bitflags() &
      rpc::header_bitflags::header_bitflags_has_dynamic_headers) {
    auto ospec = find_failure_spec(ctx.payload_headers());
    if (ospec) {
      rpc::failure_spec fspec = std::move(ospec.value());
      auto typestr = rpc::EnumNamefailure_spec_type(fspec.ftype());
      LOG_INFO("Received failure spec: {}", typestr);
      switch (fspec.ftype()) {
      case rpc::failure_spec_type::failure_spec_type_terminate:
        LOG_INFO("{}: implemented as std::terminate", typestr);
        std::terminate();
      case rpc::failure_spec_type::failure_spec_type_exception:
        LOG_INFO("{}: Injecting exception", typestr);
        return seastar::make_exception_future<rpc_recv_context>(
          failure_injector_recv_filter_exception());
      case rpc::failure_spec_type::failure_spec_type_delay:
        LOG_INFO("{}: Injecting delay of {}ms", typestr,
                 fspec.delay_duration_ms());
        return seastar::sleep(
                 std::chrono::milliseconds(fspec.delay_duration_ms()))
          .then([ctx = std::move(ctx)]() mutable {
            return seastar::make_ready_future<rpc_recv_context>(std::move(ctx));
          });
      default:
        LOG_ERROR("Could not find action for failure spec type {}", typestr);
        break;
      }
    }
  }
  return seastar::make_ready_future<rpc_recv_context>(std::move(ctx));
}  // namespace smf

}  // namespace smf
