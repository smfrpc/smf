// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// seastar
#include <seastar/core/future.hh>
#include <seastar/core/iostream.hh>
// smf
#include "smf/macros.h"
#include "smf/rpc_letter.h"

namespace smf {
/// \brief send rpc request to stablished remote host.
/// This class will async send the request to the remote host in rpc_client.h
/// and will ensure that the request is valid, enabling tracing if available,
/// compression, etc. Send arbitrary byte arrays to remote host
///
struct rpc_envelope {
  constexpr static size_t kHeaderSize = sizeof(rpc::header);
  static seastar::future<> send(seastar::output_stream<char> *out,
                                rpc_envelope req);

  rpc_envelope();
  ~rpc_envelope();
  explicit rpc_envelope(rpc_letter &&_letter);

  /// \brief copy ctor deleted
  SMF_DISALLOW_COPY_AND_ASSIGN(rpc_envelope);

  /// \brief move ctor
  rpc_envelope(rpc_envelope &&o) noexcept;
  rpc_envelope &operator=(rpc_envelope &&o) noexcept;

  /// \brief add a key=value pair ala HTTP/1.1
  /// Useful for the framework to send trace information etc.
  ///
  void add_dynamic_header(const char *header, const char *value);

  /// \brief used on the client-sender side
  SMF_ALWAYS_INLINE void
  set_request_id(uint32_t request_id) {
    letter.header.mutate_meta(request_id);
  }

  /// \brief typically used on the server-returning-content side.
  /// usually it acts like the HTTP status codes
  SMF_ALWAYS_INLINE void
  set_status(const uint32_t &status) {
    letter.header.mutate_meta(status);
  }

  SMF_ALWAYS_INLINE size_t
  size() const {
    return letter.size();
  }

  /// \brief, sometimes you know/understand the lifecycle and want a read
  /// only copy of this rpc_envelope - note that headers are 'copied', the
  /// payload, however is 'shared()'
  rpc_envelope
  share() {
    return rpc_envelope(letter.share());
  }

  rpc_letter letter;
};
}  // namespace smf
