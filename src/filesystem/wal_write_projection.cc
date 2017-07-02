// Copyright 2017 Alexander Gallego
//

#include "filesystem/wal_write_projection.h"

// TODO(wrap in a codec.h file in utils w/ a .cc file too)
#define ZSTD_STATIC_LINKING_ONLY  // ZSTD_findDecompressedSize
#include <zstd.h>

#include "hashing/hashing_utils.h"
#include "platform/log.h"
#include "platform/macros.h"

namespace smf {
using namespace smf::wal;  // NOLINT
static const uint64_t kMinCompressionSize = 512;


seastar::lw_shared_ptr<wal_write_projection::item> xform(
  const tx_put_fragment &f) {
  static thread_local flatbuffers::FlatBufferBuilder fbb{};
  // unfortunatealy, we need to copy mem, then compress it :( - so 2 memcpy.
  fbb.Clear();  // MUST happen first
  auto _op       = f.op();
  auto _epoch_ms = f.epoch_ms();
  auto _type     = f.type();
  auto _kv =
    f.kv() ?
      Createtx_put_kv(
        fbb, fbb.CreateVector(f.kv()->key()->Data(), f.kv()->key()->Length()),
        fbb.CreateVector(f.kv()->value()->Data(), f.kv()->value()->Length())) :
      0;
  auto _invalidation = f.invalidation() ? smf::wal::Createtx_put_invalidation(
                                            fbb, f.invalidation()->reason(),
                                            f.invalidation()->offset()) :
                                          0;
  fbb.Finish(smf::wal::Createtx_put_fragment(fbb, _op, _type, _epoch_ms, _kv,
                                             _invalidation));
  auto retval          = seastar::make_lw_shared<wal_write_projection::item>();
  auto zstd_size_bound = ZSTD_compressBound(fbb.GetSize());
  retval->fragment =
    std::move(seastar::temporary_buffer<char>(zstd_size_bound));
  const char *source     = (const char *)fbb.GetBufferPointer();
  const char *source_end = source + fbb.GetSize();
  char *      dest       = (char *)retval->fragment.get_write();

  if (fbb.GetSize() > kMinCompressionSize) {
    auto zstd_compressed_size = ZSTD_compress((void *)dest, zstd_size_bound,
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
  const tx_put_partition_pair *req) {
  auto ret = seastar::make_lw_shared<wal_write_projection>();
  std::for_each(req->txs()->begin(), req->txs()->end(), [ret](auto it) {
    // even though it's a list, push_back is O( 1 )
    ret->projection.push_back(xform(*it));
  });
  return ret;
}

}  // namespace smf
