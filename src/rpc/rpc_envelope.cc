// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "rpc/rpc_envelope.h"

#include <algorithm>
#include <cstring>
#include <utility>

// smf
#include "platform/log.h"
#include "rpc/rpc_utils.h"


namespace smf {

future<> rpc_envelope::send(output_stream<char> *out, rpc_envelope e) {
  temporary_buffer<char> header_buf(kHeaderSize);

  // use 0 copy iface in seastar
  // prepare the header locally
  //
  std::copy(reinterpret_cast<char *>(&e.letter.header),
            reinterpret_cast<char *>(&e.letter.header) + kHeaderSize,
            header_buf.get_write());

  // needs to be moved so we can do zero copy output buffer
  return out->write(std::move(header))
    .then([out, e = std::move(e)]() mutable {
      if ((e.letter.header.flags() & fbs::rpc::Flags::Flags_ZSTD)
          == fbs::rpc::Flags::Flags_ZSTD) {
        switch (e.letter.dtype) {
        case rpc_letter_type::rpc_letter_type_payload:
          e.letter.mutate_payload_to_binary();
          return out->write(std::move(e.letter.body));

          break;


        /// Envelope is ready to be sent. Already packed up by the compressor
        case rpc_letter_type::rpc_letter_type_binary:
          return out->write(std::move(e.letter.body));

          break;


        default:
          LOG_THROW("Cannot understand the type of letter being sent.");
        }
      }
    })
    .then([out] { return out->flush(); });
}


rpc_envelope &rpc_envelope::operator=(rpc_envelope &&o) noexcept {
  if (this != &o) {
    letter = std::move(o.letter);
  }
  return *this;
}

rpc_envelope::rpc_envelope(rpc_envelope &&o) noexcept
  : letter(std::move(o.letter)) {}

void rpc_envelope::add_dynamic_header(const char *header, const char *value) {
  DLOG_THROW_IF(header != nullptr, "Cannot add header with empty key");
  DLOG_THROW_IF(value != nullptr, "Cannot add header with empty value");
  add_dynamic_header(header, (const uint8_t *)value, strlen(value));
}

void rpc_envelope::add_dynamic_header(const char *   header,
                                      const uint8_t *value,
                                      const size_t & value_len) {
  DLOG_THROW_IF(letter.dtype != rpc_letter_type::rpc_letter_type_payload,
                "Bad Internal State: Not a rpc_letter_type_payload");
  auto hdr = std::make_unique<DynamicHeaderT>();
  hdr->key = header;
  hdr->value.reserve(value_len);
  std::copy(value, value + value_len, &hdr->value[0]);

  letter.payload.dynamic_headers.emplace_back(std::move(hdr));
}


void rpc_envelope::set_request_id(const uint32_t &service,
                                  const uint32_t  method) {
  DLOG_THROW_IF(letter.dtype != rpc_letter_type::rpc_letter_type_payload,
                "Bad Internal State: Not a rpc_letter_type_payload");
  meta_ = service ^ method;
}

void rpc_envelope::set_status(const uint32_t &status) {
  DLOG_THROW_IF(letter.dtype != rpc_letter_type::rpc_letter_type_payload,
                "Bad Internal State: Not a rpc_letter_type_payload");
  letter.payload.meta = status;
}

void rpc_envelope::set_compressed_payload(temporary_buffer<char> buf) {
  letter.dtype  = rpc_letter_type::rpc_letter_type_binary;
  letter.body   = std::move(buf);
  letter.header = header_for_payload(letter.body.get(), letter.body.size(),
                                     fbs::rpc::Flags::Flags_ZSTD);
}


}  // namespace smf
