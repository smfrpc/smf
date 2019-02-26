// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "smf/zstd_filter.h"

#include <utility>

#include "smf/compression.h"
#include "smf/log.h"
#include "smf/rpc_header_utils.h"
#include "smf/rpc_recv_context.h"

namespace smf {

static thread_local auto compressor =
  codec::make_unique(codec_type::zstd, compression_level::fastest);

seastar::future<zstd_compression_filter::type>
zstd_compression_filter::operator()(zstd_compression_filter::type &&e) {
  if (!e) { return seastar::make_ready_future<type>(std::move(e)); }
  if (e->letter.header.compression() !=
      rpc::compression_flags::compression_flags_none) {
    return seastar::make_ready_future<rpc_envelope>(std::move(e));
  }
  if (e->letter.body.size() <= min_compression_size) {
    return seastar::make_ready_future<type>(std::move(e));
  }

  e->letter.body = compressor->compress(e->letter.body);
  e->letter.header.mutate_compression(
    rpc::compression_flags::compression_flags_zstd);
  checksum_rpc(e->letter.header, e->letter.body.get(), e->letter.body.size());

  return seastar::make_ready_future<rpc_envelope>(std::move(e));
}

seastar::future<zstd_decompression_filter::type>
zstd_decompression_filter::operator()(zstd_decompression_filter::type &&ctx) {
  if (!ctx) { return seastar::make_ready_future<type>(std::move(ctx)); }
  if (ctx->header.compression() ==
      rpc::compression_flags::compression_flags_zstd) {
    ctx->payload = compressor->uncompress(ctx.payload);
    ctx->header.mutate_compression(
      rpc::compression_flags::compression_flags_none);
    checksum_rpc(ctx->header, ctx->payload.get(), ctx->payload.size());
  }
  return seastar::make_ready_future<type>(std::move(ctx));
}

}  // namespace smf
