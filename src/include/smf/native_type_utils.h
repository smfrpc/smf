// Copyright 2017 Alexander Gallego
//

#pragma once

#include <flatbuffers/flatbuffers.h>
#include <core/print.hh>
#include <core/temporary_buffer.hh>

#include "smf/flatbuffers_concepts.h"
#include "smf/log.h"

namespace smf {
/// \brief converts a flatbuffers::NativeTableType into a buffer.
/// See full benchmarks/comparisons here:
/// https://github.com/senior7515/smf/pull/259
///
/// Makes 2 copies in the worst case. See PR #259
/// If the buffer that we encode is large, we generate a new fbs::builder
/// to keep memory usage predicatable.
///
///
template <typename RootType>
SMF_CONCEPT(requires FlatBuffersNativeTable<RootType>)
seastar::temporary_buffer<char> native_table_as_buffer(
  const typename RootType::NativeTableType &t) {
  static thread_local auto builder =
    std::make_unique<flatbuffers::FlatBufferBuilder>();

  auto &bdr = *builder.get();
  bdr.Clear();
  bdr.Finish(RootType::Pack(bdr, &t, nullptr));

  if (SMF_UNLIKELY(bdr.GetSize() > 2048)) {
    auto mem = bdr.Release();
    auto ptr = reinterpret_cast<char *>(mem.data());
    auto sz = mem.size();

    // fix the original builder
    builder = std::make_unique<flatbuffers::FlatBufferBuilder>();

    return seastar::temporary_buffer<char>(
      ptr, sz, seastar::make_object_deleter(std::move(mem)));
  }


  // always allocate to the largest member 8-bytes
  void *ret = nullptr;
  auto r = posix_memalign(&ret, 8, bdr.GetSize());
  if (r == ENOMEM) {
    throw std::bad_alloc();
  } else if (r == EINVAL) {
    throw std::runtime_error(seastar::sprint(
      "Invalid alignment of %d; allocating %d bytes", 8, bdr.GetSize()));
  }
  DLOG_THROW_IF(r != 0,
    "ERRNO: {}, Bad aligned allocation of {} with alignment: {}", r,
    bdr.GetSize(), 8);
  seastar::temporary_buffer<char> retval(reinterpret_cast<char *>(ret),
    bdr.GetSize(), seastar::make_free_deleter(ret));
  std::memcpy(retval.get_write(),
    reinterpret_cast<const char *>(bdr.GetBufferPointer()), retval.size());
  return std::move(retval);
}

}  // namespace smf

// Reset()
/*
  buf.Reset() -> deallocs
  buf.reserve_size(1024) - reallocs?
 */
