#pragma once

#include "platform/log.h"
#include "rpc/rpc_server.h"
#include "rpc/rpc_server_stats_printer.h"

namespace smf {

struct distributed_broker {
  // distributed<smf::rpc_server_stats> rpc_stats;
  // distributed<smf::rpc_server>       rpc;
  // distributed<smf::write_ahead_log>  log;
  // smf::rpc_server_stats_printer      rpc_printer(&rpc_stats);

  future<> start(const boost::program_options::variables_map &config) {
    return make_ready_future<>();
    // uint16_t port = config["port"].as<uint16_t>();
    // return rpc_stats.start()
    //   .then([this] {
    //     LOG_INFO("Starting stats");
    //     rpc_printer.start();
    //     return make_ready_future<>();
    //   })
    //   .then([port] {
    //     uint32_t flags = rpc_server_flags::rpc_server_flags_none;
    //     return rpc.start(&stats, port, flags).then([&rpc] {
    //       LOG_INFO("Registering smf::chains::chain_replication_service");
    //       return rpc.invoke_on_all(
    //         &smf::rpc_server::
    //           register_service<smf::chains::chain_replication_service>);
    //     });
    //   })
    //   .then([&rpc] {
    //     /// example using a struct template
    //     return rpc.invoke_on_all(&smf::rpc_server::register_incoming_filter,
    //                              smf::zstd_decompression_filter());
    //   })
    //   .then([&rpc] {
    //     LOG_INFO("Invoking rpc start on all cores");
    //     return rpc.invoke_on_all(&smf::rpc_server::start);
    //   });
  }

  future<> stop() {
    LOG_INFO("Stopping stats");
    // return stats.stop().then([this] { return rpc.stop(); }).then([this] {
    //   return log.stop();
    // });
    return make_ready_future<>();
  }
};


}  // namespace smf
