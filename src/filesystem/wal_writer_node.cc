#include "filesystem/wal_writer_node.h"
// third party
#include <core/reactor.hh>
#include <core/file.hh>
#include <core/fstream.hh>
#include <fmt/format.h>
#include <zstd.h>

// smf
#include "hashing_utils.h"
#include "log.h"
#include "likely.h"
#include "filesystem/wal_writer_utils.h"

namespace smf {

static const size_t kWalHeaderSize = sizeof(fbs::wal::wal_header);

wal_writer_node::wal_writer_node(sstring prefix,
                                 uint64_t epoch,
                                 uint64_t _min_compression_size,
                                 const uint64_t file_size)
  : prefix_name(prefix)
  , max_size(file_size)
  , min_compression_size(_min_compression_size)
  , epoch_(epoch) {
  if(file_size < min_entry_size()) {
    LOG_THROW("file size `{}` cannot be smaller than min_entry_size `{}`",
              file_size, min_entry_size());
  }
  opts_.buffer_size = file_size;
  opts_.preallocation_size = file_size;
}

future<> wal_writer_node::open() {
  const auto name = wal_file_name(prefix_name, epoch_);
  LOG_INFO("Creating new WAL file {}", name);
  // the file should fail if it exists. It should not exist on disk, as
  // we'll truncate them
  return open_file_dma(name, open_flags::rw | open_flags::create
                               | open_flags::truncate | open_flags::exclusive)
    .then([this](file f) {
      fstream_ = make_file_output_stream(std::move(f), opts_);
      return make_ready_future<>();
    });
}


temporary_buffer<char> header_as_buffer(fbs::wal::wal_header h) {
  LOG_INFO("header_as_buffer");

  temporary_buffer<char> header(kWalHeaderSize);
  std::copy((char *)&h, (char *)&h + kWalHeaderSize, header.get_write());
  return std::move(header);
}

future<> wal_writer_node::do_append_with_header(fbs::wal::wal_header h,
                                                temporary_buffer<char> &&buf) {
  LOG_INFO("do_append_with_header");
  current_size_ += kWalHeaderSize;
  return fstream_.write(header_as_buffer(h))
    .then([this, buf = std::move(buf)]() mutable {
      current_size_ += buf.size();
      return fstream_.write(std::move(buf));
    });
}


future<> wal_writer_node::do_append(temporary_buffer<char> &&buf,
                                    fbs::wal::wal_entry_flags flags) {
  LOG_INFO("do_append");
  fbs::wal::wal_header hdr;
  hdr.mutate_flags(flags);
  auto const write_size = buf.size();
  assert(write_size <= space_left());
  if(write_size <= min_compression_size) {
    current_size_ += write_size;
    hdr.mutate_size(write_size);
    hdr.mutate_checksum(xxhash_64(buf.get(), buf.size()));
    return do_append_with_header(hdr, std::move(buf));
  }
  temporary_buffer<char> compressed_buf(write_size);
  void *dst = static_cast<void *>(compressed_buf.get_write());
  const void *src = reinterpret_cast<const void *>(buf.get());
  auto zstd_compressed_size = ZSTD_compress(dst, write_size, src, write_size,
                                            3 /*default compression level*/);

  auto zstd_err = ZSTD_isError(zstd_compressed_size);
  if(likely(zstd_err == 0)) {
    compressed_buf.trim(zstd_compressed_size);
    flags = (fbs::wal::wal_entry_flags)(
      (uint32_t)flags
      | (uint32_t)fbs::wal::wal_entry_flags::wal_entry_flags_zstd);
    hdr.mutate_flags(flags);
    hdr.mutate_size(zstd_compressed_size);
    hdr.mutate_checksum(xxhash_64(buf.get(), buf.size()));
    return do_append_with_header(hdr, std::move(compressed_buf));
  }
  LOG_THROW("Error compressing zstd buffer. defaulting to uncompressed. "
            "Code: {}, Desciption: {}",
            zstd_err, ZSTD_getErrorName(zstd_err));
}

future<> wal_writer_node::append(temporary_buffer<char> &&buf) {
  auto const write_size = buf.size();
  if(likely(write_size < space_left() && min_entry_size() < space_left())) {
    return do_append(std::move(buf),
                     fbs::wal::wal_entry_flags::wal_entry_flags_full_frament);
  }
  if(space_left() < min_entry_size()) {
    temporary_buffer<char> zero(space_left());
    std::fill(zero.get_write(), zero.get_write() + zero.size(), 0);
    return fstream_.write(std::move(zero))
      .then([this, buf = std::move(buf)]() mutable {
        return rotate_fstream().then([this, buf = std::move(buf)]() mutable {
          return append(std::move(buf));
        });
      });
  }
  // at least min_entry_size() left,
  auto share_size =
    std::min(space_left() - kWalHeaderSize, write_size + kWalHeaderSize);
  auto tmp = buf.share(0, share_size);
  buf.trim_front(share_size);
  return do_append(std::move(tmp),
                   fbs::wal::wal_entry_flags::wal_entry_flags_partial_fragment)
    .then([this, buf = std::move(buf)]() mutable {
      return append(std::move(buf));
    });
}

future<> wal_writer_node::close() {
  assert(!closed_);
  closed_ = true;
  return fstream_.flush().then([this] { return fstream_.close(); });
}
wal_writer_node::~wal_writer_node() {
  if(!closed_) {
    const auto name = wal_file_name(prefix_name, epoch_);
    LOG_ERROR("File {} was not closed, possible data loss", name);
  }
}
future<> wal_writer_node::rotate_fstream() {
  return fstream_.flush().then([this] {
    return fstream_.close().then([this] {
      epoch_ += max_size;
      current_size_ = 0;
      return open();
    });
  });
}


} // namespace smf
