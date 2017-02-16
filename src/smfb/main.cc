// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <iostream>
// seastar
#include <core/app-template.hh>
// smf
#include "chain_replication/chain_replication_service.h"
#include "filesystem/wal.h"
#include "platform/log.h"
#include "rpc/rpc_server.h"
#include "rpc/rpc_server_stats_printer.h"
// smfb
#include "smfb/smfb_command_line_options.h"

int main(int argc, char **argv, char **env) {
  distributed<smf::rpc_server_stats> stats;
  distributed<smf::rpc_server>       rpc;
  smf::rpc_server_stats_printer      stats_printer(&stats);
  app_template                       app;
  smf::smfb_add_command_line_options(&app.configuration(), argc, argv);
  return app.run_deprecated(argc, argv, [&] {
    SET_LOG_LEVEL(seastar::log_level::debug);
    LOG_INFO("Setting up at_exit hooks");
    engine().at_exit([&] { return rpc.stop(); });
    engine().at_exit([&] { return stats.stop(); });
    auto &&  config = app.configuration();
    uint16_t port   = config["port"].as<uint16_t>();
    LOG_INFO("starting stats");
    return stats.start()
      .then([&stats_printer] {
        LOG_INFO("Starting stats");
        stats_printer.start();
        return make_ready_future<>();
      })
      .then([&rpc, &stats, port] {
        uint32_t flags = smf::RPCFLAGS::RPCFLAGS_LOAD_SHEDDING_ON;
        return rpc.start(&stats, port, flags).then([&rpc] {
          LOG_INFO("Registering smf::chains::chain_replication_service");
          return rpc.invoke_on_all(
            &smf::rpc_server::
              register_service<smf::chains::chain_replication_service>);
        });
      })
      .then([&rpc] {
        /// example using a struct template
        return rpc.invoke_on_all(&smf::rpc_server::register_incoming_filter,
                                 smf::zstd_decompression_filter());
      })
      .then([&rpc] {
        LOG_INFO("Invoking rpc start on all cores");
        return rpc.invoke_on_all(&smf::rpc_server::start);
      });
  });
  return 0;
}
