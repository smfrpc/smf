// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// smf
#include "smf/rpc_envelope.h"
#include "smf/rpc_filter.h"
#include "smf/rpc_recv_context.h"

namespace smf {

struct zstd_compression_filter : rpc_filter<rpc_envelope> {
  explicit zstd_compression_filter(uint32_t _min_compression_size)
    : min_compression_size(_min_compression_size) {}

  seastar::future<rpc_envelope> operator()(rpc_envelope &&e);

  const uint32_t min_compression_size;
};

struct zstd_decompression_filter : rpc_filter<rpc_envelope> {
  seastar::future<rpc_recv_context> operator()(rpc_recv_context &&ctx);
};

}  // namespace smf
