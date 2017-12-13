// Copyright 2017 Alexander Gallego
//

#pragma once

#include <core/temporary_buffer.hh>
#include <flatbuffers/flatbuffers.h>

#include "rpc/flatbuffers_concepts.h"

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
  seastar::temporary_buffer<char> retval(builder.GetSize());
  std::memcpy(retval.get_write(),
              reinterpret_cast<const char *>(builder.GetBufferPointer()),
              retval.size());
  return std::move(retval);
}

}  // namespace smf
