// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_letter.h"

namespace smf {

rpc_letter::rpc_letter() {}

rpc_letter &rpc_letter::operator=(rpc_letter &&l) {
  header          = std::move(l.header);
  dynamic_headers = std::move(l.dynamic_headers);
  body            = std::move(l.body);
  return *this;
}

rpc_letter::rpc_letter(rpc_letter &&l) { *this = std::move(l); }

rpc_letter::~rpc_letter() {}

size_t rpc_letter::size() const { return sizeof(header) + body.size(); }
bool   rpc_letter::empty() const { return body.size() == 0; }

}  // namespace smf
