// std
#include <iostream>
// seastar
#include <core/app-template.hh>
// smf
#include "rpc/rpc_server.h"
#include "rpc/rpc_stats_printer.h"

namespace bpo = boost::program_options;

int main(int args, char **argv, char **env) {
  distributed<smf::rpc_stats> stats;
  distributed<smf::rpc_server> rpc;
  smf::rpc_stats_printer stats_printer(stats);
  app_template app;
  // TODO(agallego) -
  // add options for printer frequency, etc
  app.add_options()("rpc_port", bpo::value<uint16_t>()->default_value(11225),
                    "rpc port");
  return app.run_deprecated(args, argv, [&] {
    engine().at_exit([&] { return rpc.stop(); });
    engine().at_exit([&] { return stats.stop(); });


    auto &&config = app.configuration();
    uint16_t port = config["rpc_port"].as<uint16_t>();
    return stats.start()
      .then([&rpc, &stats, port] { return rpc.start(std::ref(stats), port); })
      .then([&rpc] { return rpc.invoke_on_all(&smf::rpc_server::start); });
  });
}
