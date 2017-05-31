// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_letter.h"

#include <core/sstring.hh>

namespace smf {

rpc_letter::rpc_letter() {
  dtype   = rpc_letter_type::rpc_letter_type_payload;
  payload = std::make_unique<smf::fbs::rpc::PayloadT>();
}
rpc_letter &rpc_letter::operator=(rpc_letter &&l) {
  dtype  = std::move(l.dtype);
  header = std::move(l.header);
  if (dtype == rpc_letter_type::rpc_letter_type_payload) {
    payload = std::move(l.payload);
  } else if (dtype == rpc_letter_type::rpc_letter_type_binary) {
    body = std::move(l.body);
  }
  return *this;
}
rpc_letter::rpc_letter(rpc_letter &&l) { *this = std::move(l); }

rpc_letter::~rpc_letter() {}


void rpc_letter::mutate_payload_to_binary() {
  LOG_THROW_IF(dtype == rpc_letter_type::rpc_letter_type_binary,
               "Letter already a binary array. Dataloss");

  // first sort data
  // equivalent to builder.CreateVectorOfSortedTables();
  std::sort(payload->dynamic_headers.begin(), payload->dynamic_headers.end(),
            [](auto &a, auto &b) { return a < b; });

  // clean up the builder first
  auto &builder = rpc_letter::local_builder();
  // Might want to keep a moving average of the memory usg
  // so that we can actually reclaim memory. by re-setting it to a smaller
  // buffer
  //
  builder.Clear();
  builder.Finish(smf::fbs::rpc::Payload::Pack(builder, payload.get()));

  // setup the body before
  seastar::temporary_buffer<char> tmp(builder.GetSize());
  const char *p = reinterpret_cast<const char *>(builder.GetBufferPointer());
  std::copy(p, p + builder.GetSize(), tmp.get_write());
  header  = header_for_payload(tmp.get(), tmp.size());
  dtype   = rpc_letter_type::rpc_letter_type_binary;
  body    = std::move(tmp);
  payload = nullptr;
}

}  // namespace smf
