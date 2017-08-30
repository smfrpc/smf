#include "filesystem/wal_write_projection.h"

#include <zstd.h>

#include "hashing/hashing_utils.h"
#include "platform/log.h"
#include "platform/macros.h"

namespace smf {
using namespace smf::wal;
static const uint64_t kMinCompressionSize = 512;

seastar::lw_shared_ptr<wal_write_projection::item> xform(
  const tx_put_fragment &f) {
  static thread_local flatbuffers::FlatBufferBuilder fbb{};
  // unfortunatealy, we need to copy mem, then compress it :( - so 2 memcpy.
  fbb.Clear();
  tx_put_dataUnion du;
  du.value = (void *)f.data();
  du.type  = f.data_type();

  fbb.Finish(Createtx_put_fragment(fbb, f.op(), f.client_seq(), f.epoch(),
                                   f.data_type(), du.Pack(fbb)));

  auto retval      = seastar::make_lw_shared<wal_write_projection::item>();
  retval->fragment = std::move(seastar::temporary_buffer<char>(fbb.GetSize()));
  const char *source     = (const char *)fbb.GetBufferPointer();
  const char *source_end = source + fbb.GetSize();
  char *      dest       = (char *)retval->fragment.get_write();

  if (fbb.GetSize() > kMinCompressionSize) {
    auto zstd_compressed_size = ZSTD_compress((void *)dest, fbb.GetSize(),
                                              (void *)source, fbb.GetSize(), 3);
    auto zstd_err = ZSTD_isError(zstd_compressed_size);
    if (SMF_LIKELY(zstd_err == 0)) {
      retval->fragment.trim(zstd_compressed_size);
      retval->hdr.mutable_ptr()->mutate_compression(
        wal_entry_compression_type::wal_entry_compression_type_zstd);
    } else {
      LOG_THROW("Error compressing zstd buffer. Code: {}, Desciption: {}",
                zstd_err, ZSTD_getErrorName(zstd_err));
    }
  }

  retval->hdr.mutable_ptr()->mutate_size(retval->fragment.size());
  retval->hdr.mutable_ptr()->mutate_checksum(
    xxhash_32(retval->fragment.get(), retval->fragment.size()));
  std::copy(source, source_end, dest);
  return retval;
}

seastar::lw_shared_ptr<wal_write_projection> wal_write_projection::translate(
  tx_put_partition_pair *req) {
  auto ret = seastar::make_lw_shared<wal_write_projection>();
  std::for_each(req->txs()->begin(), req->txs()->end(), [ret](auto it) {
    // even though it's a list, push_back is O( 1 )
    ret->projection.push_back(xform(*it));
  });
  return ret;
}

}  // namespace smf
