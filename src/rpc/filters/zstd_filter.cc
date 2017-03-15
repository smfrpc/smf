// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/filters/zstd_filter.h"
#include <utility>
// third party
#include <zstd.h>
// smf
#include "platform/log.h"
#include "rpc/rpc_recv_context.h"
#include "rpc/rpc_utils.h"

namespace smf {

future<rpc_envelope> zstd_compression_filter::operator()(rpc_envelope &&e) {
  if (e.payload_size() >= min_compression_size) {
    temporary_buffer<char> buf(e.payload_size());
    void *                 dst = static_cast<void *>(buf.get_write());
    const void *           src = reinterpret_cast<const void *>(e.payload());
    auto                   zstd_compressed_size = ZSTD_compress(
      dst, buf.size(), src, e.payload_size(), 3 /*default compression level*/);
    auto zstd_err = ZSTD_isError(zstd_compressed_size);
    if (zstd_err == 0) {
      buf.trim(zstd_compressed_size);
      e.set_compressed_payload(std::move(buf));
    } else {
      LOG_ERROR("Error compressing zstd buffer. defaulting to uncompressed. "
                "Code: {}, Desciption: {}",
                zstd_err, ZSTD_getErrorName(zstd_err));
    }
  }

  return make_ready_future<rpc_envelope>(std::move(e));
}


future<rpc_recv_context> zstd_decompression_filter::operator()(
  rpc_recv_context &&ctx) {
  if ((ctx.header()->flags() & fbs::rpc::Flags::Flags_ZSTD)
      == fbs::rpc::Flags::Flags_ZSTD) {
    auto zstd_size = ZSTD_getDecompressedSize(
      static_cast<const void *>(ctx.body_buf.get()), ctx.body_buf.size());
    if (zstd_size == 0) {
      LOG_ERROR("Cannot decompress. Size of the original is unknown");
      return make_ready_future<rpc_recv_context>(std::move(ctx));
    }
    temporary_buffer<char> decompressed_body(zstd_size);
    auto                   size_decompressed = ZSTD_decompress(
      static_cast<void *>(decompressed_body.get_write()), zstd_size,
      static_cast<const void *>(ctx.body_buf.get()), ctx.body_buf.size());

    if (zstd_size == size_decompressed) {
      decompressed_body.trim(size_decompressed);
      // Recompute the header
      auto flags =
        itof(ftoi(ctx.header()->flags()) & ~ftoi(fbs::rpc::Flags::Flags_ZSTD));
      *ctx.header() = header_for_payload(decompressed_body.get(),
                                       decompressed_body.size(), flags);
      ctx.body_buf = std::move(decompressed_body);
    } else {
      LOG_ERROR(
        "zstd decompression failed. Size expected: {}, decompressed size: {}",
        zstd_size, size_decompressed);
    }
  }
  return make_ready_future<rpc_recv_context>(std::move(ctx));
}


}  // namespace smf
