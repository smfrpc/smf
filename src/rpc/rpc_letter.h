// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#pragma once

#include <core/sstring.hh>
#include <core/temporary_buffer.hh>

#include "flatbuffers/rpc_generated.h"
#include "platform/macros.h"

namespace smf {

struct rpc_letter {
  rpc_letter();
  rpc_letter &operator=(rpc_letter &&l);
  rpc_letter(rpc_letter &&l);
  ~rpc_letter();
  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_letter);

  size_t size() const;
  bool   empty() const;

  rpc::header                     header;
  rpc::payload_headersT           dynamic_headers;
  seastar::temporary_buffer<char> body;
};

}  // namespace smf
