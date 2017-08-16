// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include "platform/macros.h"
#include "rpc/rpc_recv_context.h"

namespace smf {

template <typename T> auto to_lw_shared(rpc_recv_typed_context<T> &&c) {
  return seastar::make_lw_shared<rpc_recv_typed_context<T>>(std::move(c));
}

template <typename T> class rpc_recv_typed_context {
 public:
  using type           = T;
  using opt_recv_ctx_t = std::experimental::optional<rpc_recv_context>;

  rpc_recv_typed_context() : ctx(std::experimental::nullopt) {}

  explicit rpc_recv_typed_context(opt_recv_ctx_t t) : ctx(std::move(t)) {
    if (SMF_LIKELY(ctx)) {
      cache_ = flatbuffers::GetMutableRoot<T>(
        ctx.value().payload->mutable_body()->Data());
    }
  }

  rpc_recv_typed_context(rpc_recv_typed_context<T> &&o) noexcept
    : ctx(std::move(o.ctx)), cache_(std::move(o.cache_)) {}

  inline T *operator->() { return cache_; }
  inline T *get() { return cache_; }

  /// \brief returns true of this obj
  /// so that it works with
  /// \code
  ///     if(obj){}
  /// \endcode
  /// simply forward the bool operator to the option
  inline operator bool() const { return ctx.operator bool(); }
  opt_recv_ctx_t ctx;
  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_recv_typed_context);

 private:
  T *cache_ = nullptr;
};
}  // namespace smf
