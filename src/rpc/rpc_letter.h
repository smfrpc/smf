// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <unordered_map>

#include "flatbuffers/rpc_generated.h"

namespace smf {

// needed for the union
enum rpc_letter_type : uint8_t {
  rpc_letter_type_compressed,
  rpc_letter_type_fbb_builder
};


struct rpc_letter {
  rpc_letter_type dtype = payload_type::payload_type_fbb_builder;


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

  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_letter);

  // Does 2 copies.
  // First copy:  it converts it into a byte array in flatbuffers-aligned
  // format.
  // Second copy: Next it moves it into the payload
  //
  template <typename T,
            typename = std::enable_if_t<std::is_base_of<flatbuffers::NativeType,
                                                        T>::value>>
  static rpc_letter native_table_to_rpc_letter(const T &t) {
#define FBB_FN_BUILD(__type) Create##__type
    rpc_letter let;
    // clean up the builder first
    auto & builder = local_builder();
    builder.Clear();

    // first copy into this local_builder
    FBB_FN_BUILD(T)(builder, &t);

    // second copy - into user_buf
    let.payload.body.reserve(builder.GetSize());
    const char *p =
      reinterpret_cast<const char *>(builder.GetBufferPointer());
    std::copy(p, p + local_uilder.GetSize(),
              reinterpret_cast<char *>(let.payload.body[0]));
    let.user_buf = std::move(tmp);
    return std::move(let);
  }

  static flatbuffers::FlatbufferBuilder &local_builder() {
    static thread_local flatbuffers::FlatbufferBuilder fbb;
    return fbb;
  }
};

}  // namespace smf
