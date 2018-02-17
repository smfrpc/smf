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
static constexpr uint64_t    kMinCompressionSize = 512;
static constexpr std::size_t kWalHeaderSize      = sizeof(wal::wal_header);

std::unique_ptr<tx_put_binary_fragmentT>
wal_write_projection::xform(const tx_put_fragmentT &f) {
  DTRACE_PROBE(smf, wal_projection_xform);
  static thread_local flatbuffers::FlatBufferBuilder fbb;
  fbb.ForceDefaults(true);
  fbb.Clear();  // MUST happen first
  static thread_local auto compression =
    codec::make_unique(codec_type::lz4, compression_level::fastest);

  // convert fragment as actual flatbuffer

  fbb.Finish(tx_put_fragment::Pack(fbb, &f, nullptr));

  // format it as it will appear on disk
  auto            retval = std::make_unique<tx_put_binary_fragmentT>();
  wal::wal_header hdr;

  if (fbb.GetSize() < kMinCompressionSize) {
    hdr.mutate_compression(
      wal_entry_compression_type::wal_entry_compression_type_none);

    hdr.mutate_size(fbb.GetSize());
    hdr.mutate_checksum(
      xxhash_64((const char *)fbb.GetBufferPointer(), fbb.GetSize()));

    // allocate once!
    retval->data.resize(kWalHeaderSize + fbb.GetSize());
    std::memcpy(retval->data.data(), (char *)&hdr, kWalHeaderSize);
    std::memcpy(retval->data.data() + kWalHeaderSize, fbb.GetBufferPointer(),
                fbb.GetSize());

    return std::move(retval);
  }

  // slow compression path

  DTRACE_PROBE(smf, wal_projection_xform_compression);
  hdr.mutate_compression(
    wal_entry_compression_type::wal_entry_compression_type_lz4);
  auto payload =
    compression->compress((char *)fbb.GetBufferPointer(), fbb.GetSize());
  hdr.mutate_size(payload.size());
  hdr.mutate_checksum(xxhash_64(payload.get(), payload.size()));

  // allocate once! - for the second time :(
  retval->data.resize(kWalHeaderSize + payload.size());
  std::memcpy(retval->data.data(), (char *)&hdr, kWalHeaderSize);
  std::memcpy(retval->data.data() + kWalHeaderSize, payload.get(),
              payload.size());

  return std::move(retval);
}

}  // namespace smf
