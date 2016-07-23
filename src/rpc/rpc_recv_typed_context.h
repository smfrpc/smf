#pragma once
#include "rpc/rpc_recv_context.h"
namespace smf {
template <typename T> struct rpc_recv_typed_context {
  using type = T;
  using opt_recv_ctx_t = std::experimental::optional<rpc_recv_context>;

  rpc_recv_typed_context() : ctx(std::experimental::nullopt) {}
  rpc_recv_typed_context(opt_recv_ctx_t t) : ctx(std::move(t)) {}
  rpc_recv_typed_context(rpc_recv_typed_context<T> &&o) noexcept
    : ctx(std::move(o.ctx)) {}

  T *get() {
    if(ctx) {
      return flatbuffers::GetMutableRoot<T>(
        ctx.value().payload->mutable_body()->Data());
    }
    return nullptr;
  }

  /// \brief returns true of this obj
  /// so that it works with
  /// \code
  ///     if(obj){}
  /// \endcode
  /// simply forward the bool operator to the option
  operator bool() const { return ctx && ctx != std::experimental::nullopt; }
  opt_recv_ctx_t ctx;
};
}
