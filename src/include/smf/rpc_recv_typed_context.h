// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include "smf/macros.h"
#include "smf/rpc_recv_context.h"

namespace smf {

template <typename T>
class rpc_recv_typed_context {
  static_assert(std::is_base_of<flatbuffers::Table, T>::value,
                "Should ONLY be Table derived classes");

 public:
  using type = T;
  using opt_recv_ctx_t = smf::compat::optional<rpc_recv_context>;

  rpc_recv_typed_context() : ctx(smf::compat::nullopt) {}

  explicit rpc_recv_typed_context(opt_recv_ctx_t t) : ctx(std::move(t)) {
    if (SMF_LIKELY(!!ctx)) {
      auto ptr = ctx.value().payload.get_write();
      cache_ = flatbuffers::GetMutableRoot<T>(ptr);
    }
  }

  rpc_recv_typed_context(rpc_recv_typed_context<T> &&o) noexcept
    : ctx(std::move(o.ctx)), cache_(std::move(o.cache_)) {}

  rpc_recv_typed_context &
  operator=(rpc_recv_typed_context<T> &&o) noexcept {
    if (this != &o) {
      this->~rpc_recv_typed_context();
      new (this) rpc_recv_typed_context(std::move(o));
    }
    return *this;
  }

  SMF_ALWAYS_INLINE T *operator->() { return cache_; }
  SMF_ALWAYS_INLINE T *
  get() const {
    return cache_;
  }

  seastar::lw_shared_ptr<rpc_recv_typed_context<T>>
  move_to_lw_shared() {
    return seastar::make_lw_shared<rpc_recv_typed_context<T>>(
      std::move(ctx.value()));
  }

  /// \brief uses the flatbuffers verifier for payload
  /// can be expensive depending on type - traverses the full type tree
  bool
  verify_fbs() const {
    if (!this->operator bool()) { return false; }
    auto &buf = ctx.value().payload;
    flatbuffers::Verifier verifier((const uint8_t *)buf.get(), buf.size());
    return get()->Verify(verifier);
  }
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
