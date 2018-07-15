// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "smf/lz4_filter.h"

#include <utility>

#include "smf/compression.h"
#include "smf/rpc_header_utils.h"
#include "smf/rpc_recv_context.h"

namespace smf {

static thread_local auto compressor =
  codec::make_unique(codec_type::lz4, compression_level::fastest);

seastar::future<rpc_envelope>
lz4_compression_filter::operator()(rpc_envelope &&e) {
  if (e.letter.header.compression() !=
      rpc::compression_flags::compression_flags_none) {
    return seastar::make_ready_future<rpc_envelope>(std::move(e));
  }
  if (e.letter.body.size() <= min_compression_size) {
    return seastar::make_ready_future<rpc_envelope>(std::move(e));
  }

  auto buf = compressor->compress(e.letter.body);
  e.letter.body = std::move(buf);
  e.letter.header.mutate_compression(
    rpc::compression_flags::compression_flags_lz4);
  checksum_rpc(e.letter.header, e.letter.body.get(), e.letter.body.size());

  return seastar::make_ready_future<rpc_envelope>(std::move(e));
}

seastar::future<rpc_recv_context>
lz4_decompression_filter::operator()(rpc_recv_context &&ctx) {
  if (ctx.header.compression() ==
      rpc::compression_flags::compression_flags_lz4) {
    auto buf = compressor->uncompress(ctx.payload);
    ctx.payload = std::move(buf);
    ctx.header.mutate_compression(
      rpc::compression_flags::compression_flags_none);
    checksum_rpc(ctx.header, ctx.payload.get(), ctx.payload.size());
  }
  return seastar::make_ready_future<rpc_recv_context>(std::move(ctx));
}

}  // namespace smf
