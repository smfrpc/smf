#pragma once

namespace smf {

///  \brief alternatives considered for the buf was to use the
///  flatbuffers::DetachedBuffer.
///
/// The issue w / it is that all the IO in seastar uses temporary_buffer<>'s and
/// it already has easy facilities to share the buffer so that it will cleanly
/// delete the resources behind the  scenes  So we pay the memcpy once and then
/// call the tmporary_buffer<>::share(i,j) calls to get a reference to the same
/// underlying array, minimizing memory consumption
///
struct wal_write_projection {
  std::list<seastar::lw_shared_ptr<fbs_typed_buf<wal::tx_get_fragment>>>
    projection;
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_write_projection);
  // This no longer makes sense. this makes sense in the projection

  static seastar::lw_shared_ptr<wal_write_projection> translate(
    wal::tx_put_request *req) {
    static const uint64_t kMinCompressionSize = 512;
    // table tx_put_request static void add_binary_flags(fbs::wal::wal_header *
    // o,
    //     fbs::wal::wal_entry_flags f) {
    //     auto b = static_cast<uint32_t>(o->flags()) |
    //     static_cast<uint32_t>(f);
    //     o->mutate_flags(static_cast<fbs::wal::wal_entry_flags>(b));
    // }{
    // topic:       string;
    // partition:   uint; // xxhash32(topic,key)
    // txs:         [tx_put_fragment];
    // }



  }
};

}  // namespace smf

/*

seastar::future<wal_write_reply> wal_writer_node::do_append_with_header(
  fbs::wal::wal_header h, wal_write_request req) {
  h.mutate_checksum(xxhash_32(req.data.get(), req.data.size()));
  h.mutate_size(req.data.size());
  current_size_ += kWalHeaderSize;
  ++stats_.total_writes;
  stats_.total_bytes += kWalHeaderSize;
  return lease_->append((const char *)&h, kWalHeaderSize).then([
    this, req = std::move(req)
  ]() mutable {
    current_size_ += req.data.size();
    stats_.total_bytes += req.data.size();
    return lease_->append(req.data.get(), req.data.size());
  });
}

seastar::future<> wal_writer_node::do_append_with_flags(
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
  seastar::temporary_buffer<char> compressed_buf(req.data.size());
  void *      dst = static_cast<void *>(compressed_buf.get_write());
  const void *src = reinterpret_cast<const void *>(req.data.get());
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


 */
