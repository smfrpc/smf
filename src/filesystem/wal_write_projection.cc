#include "filesystem/wal_write_projection.h"

#include "platform/macros.h"

namespace smf {
using namespace smf::wal;
static const uint64_t kMinCompressionSize = 512;

seastar::lw_shared_ptr<wal_write_projection::item> xform(tx_put_fragment *f) {
  static thread_local flatbuffers::FlatBufferBuilder fbb{};
  // unfortunatealy, we need to copy mem, then compress it :( - so 2 memcpy.
  fbb.Clear();
  fbb.Finish(Createtx_put_fragment(fbb, f->op(), f->client_seq(),
                                   f->epoch() f->data_type(), f->data()));
  auto retval      = seastar::make_lw_shared<wal_write_projection::item>();
  retval->fragment = std::move(seastar::temporary_buffer<char>(fbb.GetSize()));
  const uint8_t *source     = fbb.GetBufferPointer();
  const uint8_t *source_end = source + fbb.GetSize();
  const uint8_t *dest       = (uint8_t *)retval->fragment.get_write();

  if (fbb.GetSize() > kMinCompressionSize) {
    auto zstd_compressed_size =
      ZSTD_compress(dest, fbb.GetSize(), source, fbb.GetSize(), 3);
    auto zstd_err = ZSTD_isError(zstd_compressed_size);
    if (SMF_LIKELY(zstd_err == 0)) {
      retval->fragment.trim(zstd_compressed_size);
      retval->hdr->mutate_compression(
        wal_entry_compression_flags::wal_entry_compression_flags_zstd);
    } else {
      LOG_THROW("Error compressing zstd buffer. Code: {}, Desciption: {}",
                zstd_err, ZSTD_getErrorName(zstd_err));
    }
  }

  retval->hdr->mutate_size(retval->fragment.size());
  retval->hdr->mutate_checksum(
    xxhash_32(retval->fragment.get(), retval->fragment.size()));
  std::copy(source, source_end, dest);
  return retval;
}

seastar::lw_shared_ptr<wal_write_projection> wal_write_projection::translate(
  wal::tx_put_request *req) {
  auto                 retval = seastar::make_lw_shared<wal_write_projection>();
  fbs::wal::wal_header hdr;
  assert(write_size <= space_left());
  if (write_size <= opts_.min_compression_size
      || (req.flags & wal_write_request_flags::wwrf_no_compression)
           == wal_write_request_flags::wwrf_no_compression) {
    return do_append_with_header(hdr, std::move(req));
  }

  return retval;
}

}  // namespace smf
