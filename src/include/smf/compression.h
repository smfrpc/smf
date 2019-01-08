// Copyright 2017 Alexander Gallego
//

#pragma once

#include <memory>

#include <seastar/core/shared_ptr.hh>
#include <seastar/core/temporary_buffer.hh>

// modeled after the folly::io::compression.h

namespace smf {

enum class codec_type { lz4, zstd };
enum class compression_level { fastest, best };

/**
 * Uncompress data. Throws std::runtime_error on decompression error.
 */
class codec {
 public:
  codec(codec_type type, compression_level level)
    : type_(type), level_(level) {}
  virtual ~codec() {}
  virtual codec_type
  type() const final {
    return type_;
  }
  virtual compression_level
  level() const final {
    return level_;
  }

  virtual seastar::temporary_buffer<char>
  compress(const seastar::temporary_buffer<char> &data) = 0;
  virtual seastar::temporary_buffer<char> compress(const char *data,
                                                   std::size_t size) = 0;

  virtual seastar::temporary_buffer<char>
  uncompress(const seastar::temporary_buffer<char> &data) = 0;
  virtual seastar::temporary_buffer<char> uncompress(const char *data,
                                                     std::size_t sz) = 0;

  static std::unique_ptr<codec> make_unique(codec_type type,
                                            compression_level level);

 private:
  codec_type type_;
  compression_level level_;
};

}  // namespace smf
