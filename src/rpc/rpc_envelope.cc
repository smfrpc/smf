// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_envelope.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include "flatbuffers/rpc_generated.h"

// smf
#include "platform/log.h"
#include "rpc/rpc_header_utils.h"


namespace smf {

seastar::future<> rpc_envelope::send(seastar::output_stream<char> *out,
                                     rpc_envelope                  e) {
  seastar::temporary_buffer<char> header_buf(kHeaderSize);
  DLOG_THROW_IF(e.letter.header.size() == 0, "Invalid computed header");
  // use 0 copy iface in seastar
  // prepare the header locally
  //
  std::copy(reinterpret_cast<char *>(&e.letter.header),
            reinterpret_cast<char *>(&e.letter.header) + kHeaderSize,
            header_buf.get_write());

  // needs to be moved so we can do zero copy output buffer
  return out->write(std::move(header_buf))
    .then([out, e = std::move(e)]() mutable {
      // TODO(agalleg) - need to see if we need to write headers
      return out->write(std::move(e.letter.body));
    })
    .then([out] { return out->flush(); });
}

rpc_envelope::rpc_envelope(rpc_letter &&l) : letter(std::move(l)) {}
rpc_envelope::~rpc_envelope() {}
rpc_envelope::rpc_envelope() {}

rpc_envelope &rpc_envelope::operator=(rpc_envelope &&o) noexcept {
  if (this != &o) { letter = std::move(o.letter); }
  return *this;
}

rpc_envelope::rpc_envelope(rpc_envelope &&o) noexcept
  : letter(std::move(o.letter)) {}

void rpc_envelope::add_dynamic_header(const char *header, const char *value) {
  DLOG_THROW_IF(header != nullptr, "Cannot add header with empty key");
  DLOG_THROW_IF(value != nullptr, "Cannot add header with empty value");
  auto hdr   = std::make_unique<rpc::dynamic_headerT>();
  hdr->key   = header;
  hdr->value = value;
  letter.dynamic_headers.dynamic_headers.emplace_back(std::move(hdr));
}

void rpc_envelope::set_request_id(const uint32_t &service,
                                  const uint32_t  method) {
  letter.header.mutate_meta(service ^ method);
}

void rpc_envelope::set_status(const uint32_t &status) {
  letter.header.mutate_meta(status);
}

}  // namespace smf
