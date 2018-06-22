// Copyright 2017 Alexander Gallego
//

#pragma once

#include <flatbuffers/flatbuffers.h>
#include <core/temporary_buffer.hh>

#include "smf/flatbuffers_concepts.h"
#include "smf/log.h"

namespace smf {
/// \brief converts a flatbuffers::NativeTableType into a buffer.
/// makes 2 copies. First the table gets copied into a flatbuffer builder
/// then it gets memcpy into a seastar returned value
///
template <typename RootType>
SMF_CONCEPT(requires FlatBuffersNativeTable<RootType>)
seastar::temporary_buffer<char> native_table_as_buffer(
  const typename RootType::NativeTableType &t) {
  static thread_local flatbuffers::FlatBufferBuilder builder;
  builder.Clear();
  builder.Finish(RootType::Pack(builder, &t, nullptr));
  // always allocate to the largest member 8-bytes
  void *ret = nullptr;
  auto r = posix_memalign(&ret, 8, builder.GetSize());
  if (r == ENOMEM) {
    throw std::bad_alloc();
  } else if (r == EINVAL) {
    throw std::runtime_error(seastar::sprint(
      "Invalid alignment of %d; allocating %d bytes", 8, builder.GetSize()));
  }
  DLOG_THROW_IF(r != 0,
    "ERRNO: {}, Bad aligned allocation of {} with alignment: {}", r,
    builder.GetSize(), 8);
  seastar::temporary_buffer<char> retval(
    (char *)ret, builder.GetSize(), seastar::make_free_deleter(ret));
  std::memcpy(retval.get_write(),
    reinterpret_cast<const char *>(builder.GetBufferPointer()), retval.size());
  return std::move(retval);
}

}  // namespace smf
