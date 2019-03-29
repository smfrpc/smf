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
  int internal_counter = 0;

  seastar::future<>
  simple_state() {
    if (internal_counter++ == 0) {
      return seastar::sleep(kMinRequestDurationOnServer);
    }
    return seastar::make_ready_future<>();
  }

  virtual seastar::future<smf::rpc_typed_envelope<smf_gen::demo::Response>>
  Get(smf::rpc_recv_typed_context<smf_gen::demo::Request> &&rec) final {
    return simple_state().then([this] {
      smf::rpc_typed_envelope<smf_gen::demo::Response> data;
      // lazy to create a new .fbs file
      data.data->name = seastar::to_sstring(internal_counter);
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
      return seastar::do_for_each(
        boost::counting_iterator<uint32_t>(0),
        boost::counting_iterator<uint32_t>(3), [=](uint32_t counter) {
          LOG_INFO("client iteration: {}", counter);
          smf::random r;
          smf::rpc_typed_envelope<smf_gen::demo::Request> req;
          req.data->name = r.next_alphanum(100);
          return client->Get(std::move(req))
            .then([](auto r) {
              const char *n = r->name()->c_str();
              int server_counter = std::stoi(n);
              LOG_THROW_IF(server_counter == 1,
                           "The first try we are supposed to timeout");
              LOG_INFO("Success. Reconnected and method returned. Server "
                       "Iteration: {}",
                       server_counter);
            })
            .handle_exception(
              [counter, client](auto err) { return client->reconnect(); });
        });
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
