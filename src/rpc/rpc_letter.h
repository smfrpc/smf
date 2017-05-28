// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#pragma once

#include <unordered_map>

#include <core/temporary_buffer.hh>

#include <flatbuffers/flatbuffers.h>

#include "flatbuffers/rpc_generated.h"
#include "platform/log.h"
#include "platform/macros.h"

namespace smf {

// needed for the union
enum rpc_letter_type : uint8_t {
  rpc_letter_type_payload,
  rpc_letter_type_binary
};


struct rpc_letter {
  rpc_letter_type dtype = rpc_letter_type::rpc_letter_type_payload;

  fbs::rpc::Header header = {0, fbs::rpc::Flags::Flags_NONE, 0};


  union {
    // used by HTTP-like payloads. get the builder, set the headers, etc
    // This is the normal use case.
    // eventually this will turn into the `this->body`
    // by the RPC before sending it over the wire
    //
    smf::fbs::rpc::PayloadT payload;
    // Used by filters / automation / etc
    // contains ALL the body of the data. That is to say
    // smf.fbs.rpc.Payload data type.
    // including headers, et al
    //
    // Typically only `smf` internals will use this.
    // The receiving end will close/abort/etc the connection if this buffer
    // is not correctly formatted as a smf.fbs.rpc.Payload table.
    //
    temporary_buffer<char> body;
  };

  rpc_letter(){}
  rpc_letter &operator=(rpc_letter &&l) {
    dtype = std::move(l.dtype);
    if (dtype == rpc_letter_type::rpc_letter_type_payload) {
      payload = std::move(l.payload);
    } else if (dtype == rpc_letter_type::rpc_letter_type_binary) {
      body = std::move(l.body);
    }
    return *this;
  }
  explicit rpc_letter(rpc_letter &&l) {
    *this = std::move(l);
  }

  ~rpc_letter(){}

  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_letter);


  void mutate_payload_to_binary() {
    LOG_THROW_IF(dtype == rpc_letter_type::rpc_letter_type_binary,
                 "Letter already a binary array. Dataloss");

    // clean up the builder first
    auto &builder = local_builder();
    // Might want to keep a moving average of the memory usg
    // so that we can actually reclaim memory. by re-setting it to a smaller
    // buffer
    //
    builder.Clear();
    CreatePayload(builder, &payload);

    // setup the body before
    temporary_buffer<char> tmp(builder.GetSize());
    const char *p = reinterpret_cast<const char *>(builder.GetBufferPointer());
    std::copy(p, p + builder.GetSize(), tmp.get_write());
    dtype = rpc_letter_type::rpc_letter_type_binary;
    body  = std::move(tmp);
  }

  // Does 2 copies.
  // First copy:  it converts it into a byte array in flatbuffers-aligned
  // format.
  // Second copy: Next it moves it into the payload
  //
  template <
    typename T,
    typename =
      std::enable_if_t<std::is_base_of<flatbuffers::NativeTable, T>::value>>
  static rpc_letter native_table_to_rpc_letter(const T &t) {
#define FBB_FN_BUILD(__type) Create##__type
    rpc_letter let;
    // clean up the builder first
    auto &builder = local_builder();
    // Might want to keep a moving average of the memory usg
    // so that we can actually reclaim memory. by re-setting it to a smaller
    // buffer
    //
    builder.Clear();

    // first copy into this local_builder
    FBB_FN_BUILD(T)(builder, &t);

    // second copy - into user_buf
    let.payload.body.reserve(builder.GetSize());
    const char *p = reinterpret_cast<const char *>(builder.GetBufferPointer());
    std::copy(p, p + builder.GetSize(),
              reinterpret_cast<char *>(&let.payload.body[0]));
    return std::move(let);
  }

  static ::flatbuffers::FlatBufferBuilder &local_builder() {
    static thread_local flatbuffers::FlatBufferBuilder fbb;
    return fbb;
  }
};

}  // namespace smf
