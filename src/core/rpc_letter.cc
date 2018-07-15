// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#include "smf/rpc_letter.h"

namespace smf {

rpc_letter::rpc_letter() {}
rpc_letter::rpc_letter(
  rpc::header _h, std::unordered_map<seastar::sstring, seastar::sstring> _hdrs,
  seastar::temporary_buffer<char> _buf)
  : header(_h), dynamic_headers(std::move(_hdrs)), body(std::move(_buf)) {}

rpc_letter &
rpc_letter::operator=(rpc_letter &&l) noexcept {
  header = l.header;
  dynamic_headers = std::move(l.dynamic_headers);
  body = std::move(l.body);
  return *this;
}
rpc_letter
rpc_letter::share() {
  return rpc_letter(header, dynamic_headers, body.share());
}

rpc_letter::rpc_letter(rpc_letter &&o) noexcept
  : header(o.header), dynamic_headers(std::move(o.dynamic_headers)),
    body(std::move(o.body)) {}

rpc_letter::~rpc_letter() {}

size_t
rpc_letter::size() const {
  return sizeof(header) + body.size();
}
bool
rpc_letter::empty() const {
  return body.size() == 0;
}

}  // namespace smf
