// Copyright 2017 Alexander Gallego
//

#include "filesystem/wal_write_projection.h"

#include <sys/sdt.h>

#include "flatbuffers/native_type_utils.h"
#include "hashing/hashing_utils.h"
#include "platform/log.h"
#include "platform/macros.h"
#include "utils/compression.h"

namespace smf {
using namespace smf::wal;  // NOLINT
static const uint64_t kMinCompressionSize = 512;


static std::unique_ptr<wal_write_projection::item>
xform(const tx_put_fragment &f) {
  DTRACE_PROBE(smf, wal_projection_xform);
  static thread_local flatbuffers::FlatBufferBuilder fbb;
  fbb.ForceDefaults(true);
  fbb.Clear();  // MUST happen first
  static thread_local auto compression =
    codec::make_unique(codec_type::lz4, compression_level::fastest);
  // unfortunatealy, we need to copy mem, then compress it :( - so 2 memcpy.
  //
  std::unique_ptr<tx_put_fragmentT> xx(f.UnPack());
  fbb.Finish(tx_put_fragment::Pack(fbb, xx.get()));
  seastar::temporary_buffer<char> payload;
  wal::wal_header                 hdr;
  if (fbb.GetSize() > kMinCompressionSize) {
    DTRACE_PROBE(smf, wal_projection_xform_compression);
    payload =
      compression->compress((char *)fbb.GetBufferPointer(), fbb.GetSize());
    hdr.mutate_compression(
      wal_entry_compression_type::wal_entry_compression_type_lz4);
  } else {
    payload = seastar::temporary_buffer<char>(fbb.GetSize());
    std::memcpy(payload.get_write(), fbb.GetBufferPointer(), fbb.GetSize());
    hdr.mutate_compression(
      wal_entry_compression_type::wal_entry_compression_type_none);
  }
  hdr.mutate_size(payload.size());
  hdr.mutate_checksum(xxhash_32(payload.get(), payload.size()));

  return std::make_unique<wal_write_projection::item>(std::move(hdr),
                                                      std::move(payload));
}

seastar::lw_shared_ptr<wal_write_projection>
wal_write_projection::translate(const seastar::sstring &      topic,
                                const tx_put_partition_tuple *req) {
  DTRACE_PROBE(smf, wal_translate);
  auto ret =
    seastar::make_lw_shared<wal_write_projection>(topic, req->partition());
  ret->projection.reserve(req->txs()->size());
  for (auto i : *req->txs()) { ret->projection.push_back(xform(*i)); }
  return ret;
}

}  // namespace smf
