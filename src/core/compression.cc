// Copyright 2017 Alexander Gallego
//

#include "smf/compression.h"

#define ZSTD_STATIC_LINKING_ONLY
#include <lz4.h>
#include <lz4hc.h>
#include <zstd.h>
#if LZ4_VERSION_NUMBER >= 10301
#include <lz4frame.h>
#endif
// The value comes from lz4.h in lz4-r117, but older versions of lz4 don't
// define LZ4_MAX_INPUT_SIZE (even though the max size is the same), so do it
// here.
#ifndef LZ4_MAX_INPUT_SIZE
#define LZ4_MAX_INPUT_SIZE 0x7E000000
#endif

#include <seastar/core/byteorder.hh>

#include "smf/log.h"
#include "smf/macros.h"

namespace smf {

class zstd_codec final : public codec {
 public:
  ~zstd_codec() {}
  zstd_codec(codec_type type, compression_level level) : codec(type, level) {}

  virtual seastar::temporary_buffer<char>
  uncompress(const seastar::temporary_buffer<char> &data) final {
    return uncompress(data.get(), data.size());
  }

  virtual seastar::temporary_buffer<char>
  uncompress(const char *data, std::size_t sz) final {
    auto zstd_size =
      ZSTD_findDecompressedSize(static_cast<const void *>(data), sz);

    LOG_THROW_IF(zstd_size == ZSTD_CONTENTSIZE_ERROR,
                 "Cannot decompress. Not compressed by zstd");
    LOG_THROW_IF(zstd_size == ZSTD_CONTENTSIZE_UNKNOWN,
                 "Cannot decompress. Unknown payload size");

    seastar::temporary_buffer<char> new_body(zstd_size);

    auto size_decompressed =
      ZSTD_decompress(static_cast<void *>(new_body.get_write()), zstd_size,
                      static_cast<const void *>(data), sz);

    LOG_THROW_IF(
      zstd_size != size_decompressed,
      "zstd decompression failed. Size expected: {}, decompressed size: {}",
      zstd_size, size_decompressed);

    return new_body;
  }

  virtual seastar::temporary_buffer<char>
  compress(const seastar::temporary_buffer<char> &data) final {
    return compress(data.get(), data.size());
  }

  virtual seastar::temporary_buffer<char>
  compress(const char *data, std::size_t sz) final {
    auto const body_size = ZSTD_compressBound(sz);

    seastar::temporary_buffer<char> buf(body_size);

    // prepare inputs
    void *dst = static_cast<void *>(buf.get_write());
    const void *src = reinterpret_cast<const void *>(data);

    // create compressed buffers
    auto zstd_compressed_size =
      ZSTD_compress(dst, buf.size(), src, sz, 3 /*default compression level*/);
    // check erros
    auto zstd_err = ZSTD_isError(zstd_compressed_size);
    LOG_THROW_IF(zstd_err != 0,
                 "Error compressing zstd buffer. defaulting to uncompressed. "
                 "Code: {}, Desciption: {}",
                 zstd_err, ZSTD_getErrorName(zstd_err));

    buf.trim(zstd_compressed_size);
    return buf;
  }
};

// Note lz4 funcs are opposite from zstd function on input->output args
// encodes 4 bytes first
class lz4_fast_codec final : public codec {
 public:
  ~lz4_fast_codec() {}
  lz4_fast_codec(codec_type type, compression_level level)
    : codec(type, level) {}

  virtual seastar::temporary_buffer<char>
  compress(const seastar::temporary_buffer<char> &data) final {
    return compress(data.get(), data.size());
  }

  virtual seastar::temporary_buffer<char>
  compress(const char *data, std::size_t size) final {
    const int max_dst_size = LZ4_compressBound(size);

    seastar::temporary_buffer<char> buf(max_dst_size + 4);

    const int compressed_data_size =
      LZ4_compress_default(data, buf.get_write() + 4, size, max_dst_size);

    LOG_THROW_IF(compressed_data_size < 0, ,
                 "A negative result from LZ4_compress_default indicates a "
                 "failure trying to compress the data.  See exit code {} "
                 "for value returned.",
                 compressed_data_size);

    LOG_THROW_IF(compressed_data_size == 0,
                 "A result of 0 means compression worked, but was stopped "
                 "because the destination buffer couldn't hold all the "
                 "information.");

    seastar::write_le<uint32_t>(buf.get_write(), size);
    buf.trim(compressed_data_size + 4);
    return buf;
  }

  virtual seastar::temporary_buffer<char>
  uncompress(const seastar::temporary_buffer<char> &data) final {
    return uncompress(data.get(), data.size());
  }

  virtual seastar::temporary_buffer<char>
  uncompress(const char *data, std::size_t sz) final {
    uint32_t orig = seastar::read_le<uint32_t>(data);

    seastar::temporary_buffer<char> buf(orig);

    const int decompressed_size = LZ4_decompress_safe(
      data + 4 /*src*/, buf.get_write() /*dest*/, sz - 4 /*compressed size*/,
      orig /*max decompressed size*/);
    LOG_THROW_IF(
      decompressed_size < 0,
      "A negative result from LZ4_decompress_safe indicates a "
      "failure trying to decompress the data.  See exit code "
      "{} for value returned. Expected original size: {}, compressed size: {}",
      decompressed_size, orig, sz - 4);
    LOG_THROW_IF(decompressed_size == 0,
                 "I'm not sure this function can ever return 0.  "
                 "Documentation in lz4.h doesn't indicate so.");
    buf.trim(decompressed_size);
    return buf;
  }
};

std::unique_ptr<codec>
codec::make_unique(codec_type type, compression_level level) {
  switch (type) {
  case codec_type::lz4:
    return std::make_unique<lz4_fast_codec>(type, level);
  case codec_type::zstd:
    return std::make_unique<zstd_codec>(type, level);
  default:
    LOG_THROW("Cannot find codec");
  }
}

}  // namespace smf
