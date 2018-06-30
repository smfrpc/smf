// Copyright 2017 Alexander Gallego
//

#pragma once

#include <flatbuffers/flatbuffers.h>
#include <core/temporary_buffer.hh>

#include "smf/flatbuffers_concepts.h"
#include "smf/log.h"

namespace smf {
template <typename RootType>
SMF_CONCEPT(requires FlatBuffersNativeTable<RootType>)
seastar::temporary_buffer<char> native_table_as_buffer(
  const typename RootType::NativeTableType &t) {
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(RootType::Pack(builder, &t, nullptr));
  auto mem = builder.Release();
  auto ptr = reinterpret_cast<char *>(mem.data());
  auto sz = mem.size();
  return seastar::temporary_buffer<char>(
    ptr, sz, seastar::make_object_deleter(std::move(mem)));
}

}  // namespace smf
