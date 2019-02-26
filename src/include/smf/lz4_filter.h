// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include "smf/rpc_envelope.h"
#include "smf/rpc_filter.h"
#include "smf/rpc_recv_context.h"

namespace smf {

struct lz4_compression_filter final : rpc_outoing_filter {
  using type = typename rpc_outoing_filter::type;

  explicit lz4_compression_filter(uint32_t _min_compression_size)
    : min_compression_size(_min_compression_size) {}

  seastar::future<type> operator()(type &&e);

  const uint32_t min_compression_size;
};

struct lz4_decompression_filter final : rpc_incoming_filter {
  using type = typename rpc_incoming_filter::type;

  seastar::future<type> operator()(type &&ctx);
};

}  // namespace smf
