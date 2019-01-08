// Copyright 2017 Alexander Gallego
//

#pragma once

#include <flatbuffers/flatbuffers.h>
#include <seastar/core/print.hh>
#include <seastar/core/temporary_buffer.hh>

#include "smf/log.h"

namespace smf {
/// \brief converts a flatbuffers::NativeTableType into a buffer.
/// See full benchmarks/comparisons here:
/// https://github.com/smfrpc/smf/pull/259
///
/// Makes 2 copies in the worst case. See PR #259
/// If the buffer that we encode is large, we generate a new fbs::builder
/// to keep memory usage predicatable.
///
///
template <typename RootType>
seastar::temporary_buffer<char>
native_table_as_buffer(const typename RootType::NativeTableType &t) {
  flatbuffers::FlatBufferBuilder bdr;
  bdr.Finish(RootType::Pack(bdr, &t, nullptr));
  auto mem = bdr.Release();
  auto ptr = reinterpret_cast<char *>(mem.data());
  auto sz = mem.size();
  return seastar::temporary_buffer<char>(
    ptr, sz, seastar::make_object_deleter(std::move(mem)));
}

}  // namespace smf
