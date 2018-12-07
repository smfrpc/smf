// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
// std
#include <chrono>
#include <iostream>
// seastar
#include <seastar/core/app-template.hh>
#include <seastar/core/distributed.hh>
#include <seastar/core/sleep.hh>
#include <seastar/core/sstring.hh>
#include <seastar/net/api.hh>
// smf
#include "integration_tests/non_root_port.h"
#include "smf/log.h"
#include "smf/random.h"
#include "smf/rpc_generated.h"
#include "smf/rpc_handle_router.h"
#include "smf/rpc_recv_context.h"
#include "smf/rpc_server.h"
#include "smf/time_utils.h"
// generated-templates
#include "integration_tests/demo_service.smf.fb.h"

using namespace std::chrono_literals;  // NOLINT
constexpr const uint32_t kCoreMemory = 1 << 20;
constexpr const auto kMinRequestDurationOnServer = 100ms;

class storage_service final : public smf_gen::demo::SmfStorage {
  virtual seastar::future<smf::rpc_typed_envelope<smf_gen::demo::Response>>
  Get(smf::rpc_recv_typed_context<smf_gen::demo::Request> &&rec) final {
    return seastar::sleep(1s).then([] {
      smf::rpc_typed_envelope<smf_gen::demo::Response> data;
      data.data->name = seastar::to_sstring(smf::lowres_time_now_millis());
      data.envelope.set_status(200);
      return seastar::make_ready_future<
        smf::rpc_typed_envelope<smf_gen::demo::Response>>(std::move(data));
    });
  }
};

static seastar::future<>
backpressure_request(uint16_t port) {
  smf::rpc_client_opts opts{};
  opts.server_addr = seastar::ipv4_addr{"127.0.0.1", port};
  opts.recv_timeout = 20ms;
  auto client =
    seastar::make_shared<smf_gen::demo::SmfStorageClient>(std::move(opts));
  return client->connect()
    .then([=] {
      smf::random r;
      smf::rpc_typed_envelope<smf_gen::demo::Request> req;
      req.data->name = r.next_alphanum(kCoreMemory);
      return client->Get(std::move(req))
        .then([](auto _) { LOG_THROW("SHOULD HAVE TIMED OUT"); })
        .handle_exception(
          [](auto err) { LOG_INFO("EXPECTED!!! Exception: {}", err); });
    })
    .then([client] { return client->stop().finally([client] {}); });
}

int
main(int args, char **argv, char **env) {
  seastar::distributed<smf::rpc_server> rpc;
  seastar::app_template app;
  smf::random rand;
  uint16_t random_port =
    smf::non_root_port(rand.next() % std::numeric_limits<uint16_t>::max());
  return app.run(args, argv, [&]() -> seastar::future<int> {
    seastar::engine().at_exit([&] { return rpc.stop(); });
    smf::rpc_server_args sargs;
    sargs.ip = "127.0.0.1";
    sargs.rpc_port = random_port;
    sargs.http_port =
      smf::non_root_port(rand.next() % std::numeric_limits<uint16_t>::max());
    sargs.flags |= smf::rpc_server_flags::rpc_server_flags_disable_http_server;

    // -- MAIN TEST --
    sargs.memory_avail_per_core =
      static_cast<uint64_t>(kCoreMemory + 200 /*some slack bytes*/);

    return rpc.start(sargs)
      .then([&rpc] {
        return rpc.invoke_on_all(
          &smf::rpc_server::register_service<storage_service>);
      })
      .then([&rpc] { return rpc.invoke_on_all(&smf::rpc_server::start); })
      .then([&] { return backpressure_request(random_port); })
      .then([] { return seastar::make_ready_future<int>(0); });
  });
}
