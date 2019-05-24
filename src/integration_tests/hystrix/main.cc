// Copyright 2019 SMF Authors
//

#include <chrono>

#include <fmt/format.h>
#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>  // timer::arm/re-arm here
#include <seastar/core/sleep.hh>
#include <seastar/core/timer.hh>
#include <smf/log.h>

#include "smf/reconnect_client.h"
// generated-templates
#include "integration_tests/demo_service.smf.fb.h"

static seastar::future<>
test() {
  smf::rpc_client_opts opts{};
  opts.server_addr = seastar::ipv4_addr{"127.0.0.1", 7897};
  auto client = seastar::make_shared<
    smf::reconnect_client<smf_gen::demo::SmfStorageClient>>(std::move(opts));
  return client->connect()
    .then([=] {
      LOG_THROW_IF(static_cast<std::underlying_type_t<smf::reconnect_backoff>>(
                     client->backoff()) != 1,
                   "Must be 1 second backoff");
      LOG_INFO("{}", client->backoff());
    })
    .then([client] { return client->stop().finally([client] {}); });
}

int
main(int args, char **argv, char **env) {
  std::cout.setf(std::ios::unitbuf);
  smf::app_run_log_level(seastar::log_level::trace);
  seastar::app_template app;
  return app.run(args, argv, [&] {
    return test().then([] { return seastar::make_ready_future<int>(0); });
  });
}
