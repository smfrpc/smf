// Copyright 2019 SMF Authors
//

#pragma once

#include <cstdint>

#include <seastar/core/sstring.hh>
#include <seastar/core/timer.hh>
#include <seastar/net/tls.hh>

namespace smf {
enum rpc_server_flags : uint32_t { rpc_server_flags_disable_http_server = 1 };

struct rpc_server_args {
  seastar::sstring ip = "";
  uint16_t rpc_port = 11225;
  uint16_t http_port = 33140;
  /// \brief rpc_server_flags are bitwise flags.
  ///
  uint32_t flags = 0;
  /// \ brief rpc server tls certs and keys
  ///
  seastar::shared_ptr<seastar::tls::server_credentials> credentials;
  /// \ brief The default timeout PER connection body. After we parse the
  /// header of the connection we need to make sure that we at some point
  /// receive some bytes or expire the connection.
  ///
  typename seastar::timer<>::duration recv_timeout = std::chrono::minutes(1);
  /// \brief 4GB usually. After this limit, each connection to this
  /// server-core will block until there are enough bytes free in memory to
  /// continue
  ///
  uint64_t memory_avail_per_core = uint64_t(1) << 31 /*2GB per core*/;
};

}  // namespace smf
