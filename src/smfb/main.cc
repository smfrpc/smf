// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <iostream>

#include <core/app-template.hh>

#include "chain_replication/chain_replication_service.h"
#include "filesystem/wal.h"
#include "platform/log.h"
#include "smfb/distributed_broker.h"
#include "smfb/smfb_command_line_options.h"


int main(int argc, char **argv, char **env) {
  smf::distributed_broker broker;
  app_template            app;

  try {
    LOG_INFO("Setting up command line flags");
    smf::smfb_add_command_line_options(&app.configuration(), argc, argv);

    return app.run(argc, argv, [&]() -> future<int> {
      LOG_INFO("Setting up at_exit hooks");
      engine().at_exit([&] { return broker.stop(); });

      LOG_INFO("Setting up at_exit hooks");
      return broker.start(app.configuration()).then([] {
        return make_ready_future<int>(0);
      });
    });
  } catch (...) {
    LOG_INFO("Exception while running broker: {}", std::current_exception());
    return 1;
  }
}
