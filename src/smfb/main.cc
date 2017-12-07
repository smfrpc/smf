// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <iostream>

#include <core/app-template.hh>
#include <core/prometheus.hh>

#include "chain_replication/chain_replication_service.h"
#include "filesystem/write_ahead_log.h"
#include "histogram/histogram_seastar_utils.h"
#include "histogram/unique_histogram_adder.h"
#include "platform/log.h"
#include "rpc/filters/lz4_filter.h"
#include "rpc/rpc_server.h"
#include "smfb/smfb_command_line_options.h"
#include "utils/checks/cpu.h"
#include "utils/checks/disk.h"
#include "utils/checks/memory.h"

void base_init(const boost::program_options::variables_map &config) {
  auto l = config["log-level"].as<std::string>();
  if (l == "debug") {
    SET_LOG_LEVEL(seastar::log_level::debug);
  } else if (l == "trace") {
    SET_LOG_LEVEL(seastar::log_level::trace);
  }
}

int main(int argc, char **argv, char **env) {
  std::setvbuf(stdout, nullptr, _IOLBF, 1024);

  seastar::distributed<smf::rpc_server>      rpc;
  seastar::distributed<smf::write_ahead_log> log;

  seastar::app_template app;

  try {
    LOG_INFO("Setting up command line flags");
    smf::smfb_command_line_options::add(app.add_options());

    return app.run_deprecated(argc, argv, [&] {
      LOG_INFO("Setting up at_exit hooks");
      seastar::engine().at_exit([&rpc] {
        LOG_INFO("Stopping RPC");
        return rpc.stop();
      });
      seastar::engine().at_exit([&log] {
        LOG_INFO("Stopping write_ahead_log");
        return log.stop();
      });

      LOG_INFO("Validating command line flags");
      smf::smfb_command_line_options::validate(app.configuration());

      base_init(app.configuration());
      auto &config = app.configuration();

      if (config["print-rpc-histogram-on-exit"].as<bool>()) {
        seastar::engine().at_exit([&rpc] {
          LOG_INFO("Writing rpc_server histograms");
          return rpc
            .map_reduce(smf::unique_histogram_adder(),
                        &smf::rpc_server::copy_histogram)
            .then([](auto h) {
              return smf::histogram_seastar_utils::write("server_hdr.hgrm",
                                                         std::move(h));
            });
        });
      }

      smf::checks::cpu::check();
      smf::checks::memory::check(config["developer"].as<bool>());

      LOG_INFO("Checking disk type");
      return smf::checks::disk::check(
               config["write-ahead-log-dir"].as<std::string>(),
               config["developer"].as<bool>())
        .then([&log, &config] {
          auto dir = config["write-ahead-log-dir"].as<std::string>();
          LOG_INFO("Starting write-ahead-log in: `{}`", dir);
          return log.start(smf::wal_opts(dir.c_str()));
        })
        .then([&log] { return log.invoke_on_all(&smf::write_ahead_log::open); })
        .then([&rpc, &config] {
          smf::rpc_server_args args;
          args.rpc_port              = config["port"].as<uint16_t>();
          args.ip                    = config["ip"].as<std::string>().c_str();
          args.memory_avail_per_core = static_cast<uint64_t>(
            0.8 * seastar::memory::stats().total_memory());

          return rpc.start(args);
        })
        .then([&rpc, &log] {
          LOG_INFO("Registering smf::chains::chain_replication_service");
          return rpc.invoke_on_all([&log](smf::rpc_server &server) {
            using cr = smf::chains::chain_replication_service;
            server.register_service<cr>(&log);
          });
        })
        .then([&rpc] {
          LOG_INFO("Adding lz4 de-compressor to rpc service");
          using lz4_t = smf::lz4_decompression_filter;
          return rpc.invoke_on_all(
            &smf::rpc_server::register_incoming_filter<lz4_t>);
        })
        .then([&rpc] {
          LOG_INFO("Adding lz4 compressor to rpc service");
          return rpc.invoke_on_all([](smf::rpc_server &server) {
            using lz4_t = smf::lz4_compression_filter;
            server.register_outgoing_filter<lz4_t>(1024);
          });
        })
        .then([&rpc] {
          LOG_INFO("Starting RPC");
          return rpc.invoke_on_all(&smf::rpc_server::start);
        });
    });
  } catch (...) {
    LOG_INFO("Exception while running broker: {}", std::current_exception());
    return 1;
  }
}
