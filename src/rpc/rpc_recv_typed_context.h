// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include "platform/macros.h"
#include "rpc/rpc_recv_context.h"

namespace smf {
template <typename T> struct rpc_recv_typed_context {
  using type           = T;
  using opt_recv_ctx_t = std::experimental::optional<rpc_recv_context>;

  rpc_recv_typed_context() : ctx(std::experimental::nullopt) {}
  explicit rpc_recv_typed_context(opt_recv_ctx_t t) : ctx(std::move(t)) {}
  rpc_recv_typed_context(rpc_recv_typed_context<T> &&o) noexcept
    : ctx(std::move(o.ctx)) {}

  T *get() {
    if (ctx) {
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
  operator bool() const { return ctx.operator bool(); }
  opt_recv_ctx_t ctx;

  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_recv_typed_context);
};
}  // namespace smf
