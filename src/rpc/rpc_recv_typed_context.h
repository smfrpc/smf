// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include "platform/macros.h"
#include "rpc/rpc_recv_context.h"

namespace smf {
template <typename T> struct rpc_recv_typed_context {
  using type = T;
  explicit rpc_recv_typed_context(rpc_recv_context &&c) : ctx(std::move(c)) {}
  rpc_recv_typed_context(rpc_recv_typed_context<T> &&o) noexcept
    : ctx(std::move(o.ctx)) {}

  T *get() {
    if (ctx) {
      return flatbuffers::GetMutableRoot<T>(
        ctx.payload()->mutable_body()->Data());
    }
    return nullptr;
  }

  rpc_recv_context ctx;
  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_recv_typed_context);
};
}  // namespace smf
