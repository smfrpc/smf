// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
// std
#include <chrono>
#include <iostream>
// seastar
#include <seastar/core/app-template.hh>
#include <seastar/core/distributed.hh>
#include <seastar/core/sleep.hh>
#include <seastar/net/api.hh>
// smf
#include "integration_tests/non_root_port.h"
#include "smf/log.h"
#include "smf/random.h"
#include "smf/rpc_generated.h"
#include "smf/rpc_handle_router.h"
#include "smf/rpc_recv_context.h"
#include "smf/rpc_server.h"
// templates
#include "integration_tests/demo_service.smf.fb.h"

class storage_service final : public smf_gen::demo::SmfStorage {
  virtual seastar::future<smf::rpc_typed_envelope<smf_gen::demo::Response>>
  Get(smf::rpc_recv_typed_context<smf_gen::demo::Request> &&rec) final {
    LOG_THROW("SHOULD NOT REACH THIS W/ A TIMEOUT");
  }
};

int
main(int args, char **argv, char **env) {
  DLOG_DEBUG("About to start the RPC test");
  seastar::distributed<smf::rpc_server> rpc;

  seastar::app_template app;

  smf::random rand;
  uint16_t random_port =
    smf::non_root_port(rand.next() % std::numeric_limits<uint16_t>::max());
  return app.run(args, argv, [&]() -> seastar::future<int> {
    DLOG_DEBUG("Setting up at_exit hooks");
    seastar::engine().at_exit([&] { return rpc.stop(); });

    smf::random r;
    smf::rpc_server_args sargs;
    sargs.rpc_port = random_port;
    sargs.http_port =
      smf::non_root_port(rand.next() % std::numeric_limits<uint16_t>::max());
    sargs.flags |= smf::rpc_server_flags::rpc_server_flags_disable_http_server;
    sargs.recv_timeout = std::chrono::milliseconds(1);  // immediately
    return rpc.start(sargs)
      .then([&rpc] {
        return rpc.invoke_on_all(
          &smf::rpc_server::register_service<storage_service>);
      })
      .then([&rpc] {
        DLOG_DEBUG("Invoking rpc start on all cores");
        return rpc.invoke_on_all(&smf::rpc_server::start);
      })
      .then([&random_port] {
        DLOG_DEBUG("Sening only header ane expecting timeout");
        auto local =
          seastar::socket_address(sockaddr_in{AF_INET, INADDR_ANY, {0}});

        return seastar::engine()
          .net()
          .connect(seastar::make_ipv4_address(random_port), local,
                   seastar::transport::TCP)
          .then([local](auto skt) {
            auto conn = seastar::make_lw_shared<smf::rpc_connection>(
              std::move(skt), local);

            uint32_t kHeaderSize = sizeof(smf::rpc::header);

            smf::rpc::header header;
            header.mutate_size(300);
            header.mutate_checksum(1234234);
            seastar::temporary_buffer<char> header_buf(kHeaderSize);
            std::copy(reinterpret_cast<char *>(&header),
                      reinterpret_cast<char *>(&header) + kHeaderSize,
                      header_buf.get_write());
            return conn->ostream.write(std::move(header_buf))
              .then([conn] { return conn->ostream.flush(); })
              .then([conn] {
                return seastar::sleep(std::chrono::milliseconds(10));
              })
              .finally([conn] {});
          });
      })
      .then([] {
        DLOG_DEBUG("Exiting");
        return seastar::make_ready_future<int>(0);
      });
  });
}
