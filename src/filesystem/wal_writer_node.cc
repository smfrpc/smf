// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "filesystem/wal_writer_node.h"
#include <utility>
// third party
#include <core/file.hh>
#include <core/fstream.hh>
#include <core/reactor.hh>
#include <zstd.h>
// smf
#include "filesystem/wal_writer_file_lease_impl.h"
#include "filesystem/wal_writer_utils.h"
#include "hashing/hashing_utils.h"
#include "platform/log.h"
#include "platform/macros.h"

namespace smf {

static const size_t kWalHeaderSize = sizeof(fbs::wal::wal_header);

file_output_stream_options fstream_opts(const wal_writer_node_opts &opts) {
  file_output_stream_options o;
  o.buffer_size        = opts.file_size;
  o.preallocation_size = opts.file_size;
  return o;
}

uint64_t wal_write_request_size(const wal_write_request &req) {
  return kWalHeaderSize + req.data.size();
}


wal_writer_node::wal_writer_node(wal_writer_node_opts &&opts)
  : opts_(std::move(opts)) {
  LOG_THROW_IF(opts_.file_size < min_entry_size(),
               "file size `{}` cannot be smaller than min_entry_size `{}`",
               opts_.file_size, min_entry_size());
}

future<> wal_writer_node::open() {
  const auto name = wal_file_name(opts_.prefix, opts_.epoch);
  LOG_THROW_IF(lease_ != nullptr,
               "opening new file. Previous file is unclosed");
  using lease_impl = smf::wal_writer_file_lease_impl;
  lease_           = std::make_unique<lease_impl>(name, fstream_opts(opts_));
  return lease_->open();
}

// this might not be needed. hehe.
future<> wal_writer_node::do_append_with_header(fbs::wal::wal_header h,
                                                wal_write_request    req) {
  h.mutate_checksum(xxhash_32(req.data.get(), req.data.size()));
  h.mutate_size(req.data.size());
  current_size_ += kWalHeaderSize;
  return lease_->append((const char *)&h, kWalHeaderSize).then([
    this, req = std::move(req)
  ]() mutable {
    current_size_ += req.data.size();
    return lease_->append(req.data.get(), req.data.size());
  });
}

static void add_binary_flags(fbs::wal::wal_header *    o,
                             fbs::wal::wal_entry_flags f) {
  auto b = static_cast<uint32_t>(o->flags()) | static_cast<uint32_t>(f);
  o->mutate_flags(static_cast<fbs::wal::wal_entry_flags>(b));
}

future<> wal_writer_node::do_append_with_flags(
  wal_write_request req, fbs::wal::wal_entry_flags flags) {
  fbs::wal::wal_header hdr;
  add_binary_flags(&hdr, flags);
  auto const write_size = wal_write_request_size(req);
  assert(write_size <= space_left());
  if (write_size <= opts_.min_compression_size
      || (req.flags & wal_write_request_flags::wwrf_no_compression)
           == wal_write_request_flags::wwrf_no_compression) {
    return do_append_with_header(hdr, std::move(req));
  }

  // bigger than compression size & has compression enabled

  temporary_buffer<char> compressed_buf(req.data.size());
  void *                 dst = static_cast<void *>(compressed_buf.get_write());
  const void *           src = reinterpret_cast<const void *>(req.data.get());
  // 3 - default compression level
  auto zstd_compressed_size =
    ZSTD_compress(dst, req.data.size(), src, req.data.size(), 3);

  auto zstd_err = ZSTD_isError(zstd_compressed_size);
  if (SMF_LIKELY(zstd_err == 0)) {
    compressed_buf.trim(zstd_compressed_size);
    add_binary_flags(&hdr, fbs::wal::wal_entry_flags::wal_entry_flags_zstd);
    wal_write_request compressed_req(req.flags, std::move(compressed_buf),
                                     req.pc);
    return do_append_with_header(hdr, std::move(compressed_req));
  }
  LOG_THROW("Error compressing zstd buffer. defaulting to uncompressed. "
            "Code: {}, Desciption: {}",
            zstd_err, ZSTD_getErrorName(zstd_err));
}

future<uint64_t> wal_writer_node::append(wal_write_request req) {
  const auto offset = opts_.epoch += current_size_;
  return do_append(std::move(req)).then([offset] {
    return make_ready_future<uint64_t>(offset);
  });
}

future<> wal_writer_node::do_append(wal_write_request req) {
  auto const write_size = wal_write_request_size(req);
  if (SMF_LIKELY(write_size < space_left())) {
    return do_append_with_flags(
      std::move(req), fbs::wal::wal_entry_flags::wal_entry_flags_full_frament);
  }
  if (space_left() < min_entry_size()) {
    return rotate_fstream().then([this, req = std::move(req)]() mutable {
      return do_append(std::move(req));
    });
  }
  // enough space, bigger than min_entry
  // not enough for put, so break it into parts
  // at least min_entry_size() left,
  auto              front     = req.data.size() - space_left() - kWalHeaderSize;
  wal_write_request begin     = req.share_range(0, front);
  wal_write_request remaining = req.share_range(front + 1, req.data.size());
  return do_append_with_flags(
           std::move(begin),
           fbs::wal::wal_entry_flags::wal_entry_flags_partial_fragment)
    .then([this, remaining = std::move(remaining)]() mutable {
      return do_append(std::move(remaining));
    });
}

future<> wal_writer_node::close() {
  return lease_->close().finally([this] { lease_ = nullptr; });
}
wal_writer_node::~wal_writer_node() {}
future<> wal_writer_node::rotate_fstream() {
  LOG_INFO("rotating fstream");
  // Schedule the future<> close().
  // Let the I/O scheduler close it concurrently
  auto ptr = lease_.release();
  ptr->flush().then([ptr] {
    ptr->close().finally([ptr] {
      // need to hold the ptr until the
      // filestream has closed to not leak memory
      delete ptr;
    });
  });
  opts_.epoch += current_size_;
  current_size_ = 0;
  return open();
}


}  // namespace smf
