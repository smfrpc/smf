// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/filters/zstd_filter.h"
#include <utility>
// third party
// TODO(move them to codec.h to hide this magic)
#define ZSTD_STATIC_LINKING_ONLY  // ZSTD_findDecompressedSize
#include <zstd.h>

// smf
#include "platform/log.h"
#include "rpc/rpc_header_utils.h"
#include "rpc/rpc_recv_context.h"

namespace smf {

seastar::future<rpc_envelope> zstd_compression_filter::operator()(
  rpc_envelope &&e) {
  if (e.letter.header.compression()
      != rpc::compression_flags::compression_flags_none) {
    return seastar::make_ready_future<rpc_envelope>(std::move(e));
  }
  if (e.letter.body.size() <= min_compression_size) {
    return seastar::make_ready_future<rpc_envelope>(std::move(e));
  }

  auto const body_size = ZSTD_compressBound(e.letter.body.size());
  seastar::temporary_buffer<char> buf(body_size);
  // prepare inputs
  void *      dst = static_cast<void *>(buf.get_write());
  const void *src = reinterpret_cast<const void *>(e.letter.body.get());
  // create compressed buffers
  auto zstd_compressed_size = ZSTD_compress(dst, buf.size(), src, body_size,
                                            3 /*default compression level*/);
  // check erros
  auto zstd_err = ZSTD_isError(zstd_compressed_size);
  if (zstd_err == 0) {
    buf.trim(zstd_compressed_size);
    e.letter.header.mutate_compression(
      rpc::compression_flags::compression_flags_zstd);
    e.letter.body = std::move(buf);
    checksum_rpc(e.letter.header, e.letter.body.get(), e.letter.body.size());
  } else {
    LOG_ERROR("Error compressing zstd buffer. defaulting to uncompressed. "
              "Code: {}, Desciption: {}",
              zstd_err, ZSTD_getErrorName(zstd_err));
  }

  return seastar::make_ready_future<rpc_envelope>(std::move(e));
}


seastar::future<rpc_recv_context> zstd_decompression_filter::operator()(
  rpc_recv_context &&ctx) {
  if (ctx.header.compression()
      == rpc::compression_flags::compression_flags_zstd) {
    auto zstd_size = ZSTD_findDecompressedSize(
      static_cast<const void *>(ctx.payload.get()), ctx.payload.size());

    if (zstd_size == ZSTD_CONTENTSIZE_ERROR) {
      LOG_ERROR("Cannot decompress. Not compressed by zstd");
      return seastar::make_ready_future<rpc_recv_context>(std::move(ctx));
    } else if (zstd_size == ZSTD_CONTENTSIZE_UNKNOWN) {
      LOG_ERROR("Cannot decompress. Unknown payload size");
      return seastar::make_ready_future<rpc_recv_context>(std::move(ctx));
    }

    seastar::temporary_buffer<char> new_body(zstd_size);

    auto size_decompressed = ZSTD_decompress(
      static_cast<void *>(new_body.get_write()), zstd_size,
      static_cast<const void *>(ctx.payload.get()), ctx.payload.size());

    if (zstd_size == size_decompressed) {
      new_body.trim(size_decompressed);
      ctx.header.mutate_compression(
        rpc::compression_flags::compression_flags_none);
      checksum_rpc(ctx.header, new_body.get(), new_body.size());
      ctx.payload = std::move(new_body);
    } else {
      LOG_ERROR(
        "zstd decompression failed. Size expected: {}, decompressed size: {}",
        zstd_size, size_decompressed);
    }
  }
  return seastar::make_ready_future<rpc_recv_context>(std::move(ctx));
}


}  // namespace smf
