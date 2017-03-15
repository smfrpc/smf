// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <iostream>

#include <core/app-template.hh>

#include "chain_replication/chain_replication_service.h"
#include "filesystem/wal.h"
#include "histogram/histogram_seastar_utils.h"
#include "platform/log.h"
#include "rpc/rpc_server.h"
#include "rpc/rpc_server_stats_printer.h"
#include "smfb/smfb_command_line_options.h"

void base_init(const boost::program_options::variables_map &config) {
  auto l = config["log-level"].as<std::string>();
  if (l == "debug") {
    SET_LOG_LEVEL(seastar::log_level::debug);
  } else if (l == "trace") {
    SET_LOG_LEVEL(seastar::log_level::trace);
  }
}

int main(int argc, char **argv, char **env) {
  distributed<smf::rpc_server_stats>             rpc_stats;
  distributed<smf::rpc_server>                   rpc;
  distributed<smf::write_ahead_log>              log;
  std::unique_ptr<smf::rpc_server_stats_printer> rpc_printer;


  app_template app;

  try {
    LOG_INFO("Setting up command line flags");
    smf::smfb_command_line_options::add(app.add_options());

    return app.run_deprecated(argc, argv, [&] {
      LOG_INFO("Setting up at_exit hooks");
      engine().at_exit([&rpc_printer, &rpc] {
        if (rpc_printer) {
          LOG_INFO("Stopping rpc_printer");
          return rpc_printer->stop();
        }
        return make_ready_future<>();
      });
      engine().at_exit([&rpc_stats] {
        LOG_INFO("Stopping rpc_stats");
        return rpc_stats.stop();
      });
      engine().at_exit([&rpc] {
        LOG_INFO("Stopping RPC");
        return rpc.stop();
      });
      engine().at_exit([&log] {
        LOG_INFO("Stopping write_ahead_log");
        return log.stop();
      });

      LOG_INFO("Validating command line flags");
      smf::smfb_command_line_options::validate(app.configuration());

      base_init(app.configuration());
      auto &config = app.configuration();

      if (config["print-rpc-stats"].as<bool>()) {
        using min_t     = std::chrono::minutes;
        using printer_t = smf::rpc_server_stats_printer;
        auto mins       = min_t(config["rpc-stats-period-mins"].as<uint32_t>());
        LOG_INFO("Enabling --print-rpc-stats every: {}min", mins.count());
        rpc_printer = std::make_unique<printer_t>(&rpc_stats, mins);
      }
      if (config["print-rpc-histogram-on-exit"].as<bool>()) {
        engine().at_exit([&rpc] {
          LOG_INFO("Writing rpc_server histograms");
          return rpc
            .map_reduce(adder<smf::histogram>(),
                        &smf::rpc_server::copy_histogram)
            .then([](smf::histogram h) {
              return smf::histogram_seastar_utils::write_histogram(
                "server_hdr.txt", std::move(h));
            });
        });
      }


      LOG_INFO("Setting up at_exit hooks");
      return rpc_stats.start()
        .then([&rpc_printer, &config] {
          if (config["print-rpc-stats"].as<bool>()) {
            auto rpc_period = config["rpc-stats-period-mins"].as<uint32_t>();
            LOG_INFO("Starting stats, with period:{}", rpc_period);
            rpc_printer->start();
          }
          return make_ready_future<>();
        })
        .then([&log, &config] {
          auto dir = config["write-ahead-log-dir"].as<std::string>();
          LOG_INFO("Starting write-ahead-log in: `{}`", dir);
          return log.start(smf::wal_type::wal_type_disk_with_memory_cache,
                           smf::wal_opts(dir.c_str()));
        })
        .then([&log] {
          LOG_INFO("Opening write ahead log");
          return log.invoke_on_all(&smf::write_ahead_log::open);
        })
        .then([&rpc, &rpc_stats, &config] {
          const uint16_t port  = config["port"].as<uint16_t>();
          uint32_t       flags = smf::rpc_server_flags::rpc_server_flags_none;
          LOG_INFO("Building RPC, port:{}, flags:{}", port, flags);
          return rpc.start(&rpc_stats, port, flags);
        })
        .then([&rpc, &log] {
          LOG_INFO("Registering smf::chains::chain_replication_service");
          return rpc.invoke_on_all([&log](smf::rpc_server &server) {
            using cr = smf::chains::chain_replication_service;
            server.register_service<cr>(&log);
          });
        })
        .then([&rpc] {
          LOG_INFO("Adding zstd compressor to rpc service");
          using zstd_t = smf::zstd_decompression_filter;
          return rpc.invoke_on_all(
            &smf::rpc_server::register_incoming_filter<zstd_t>);
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
