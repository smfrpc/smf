// Copyright 2019 SMF Authors
//

#include <chrono>
#include <iostream>
#include <random>

#include <seastar/core/app-template.hh>
#include <seastar/core/distributed.hh>
#include <seastar/core/metrics_registration.hh>
#include <seastar/core/prometheus.hh>
#include <seastar/http/httpd.hh>

#include "smf/histogram.h"

struct histgen_server_args {
  seastar::sstring ip = "";
  uint16_t port = 33140;
  uint64_t min_val;
  uint64_t max_val;
  uint32_t num_samples;
};

class histgen_server {
 public:
  histgen_server(histgen_server_args args)
    : args(args), hist(smf::histogram::make_lw_shared()),
      http_server("histgen server") {
    namespace sm = seastar::metrics;
    metrics.add_group(
      "histgen_server",
      {
        sm::make_histogram(
          "synthetic_histogram", sm::description("Synthetic histogram data"),
          [this] { return hist->seastar_histogram_logform(); }),
      });
  }

  ~histgen_server() {}

  seastar::future<>
  serve() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dist(args.min_val, args.max_val);
    boost::counting_iterator<int> from(0);
    boost::counting_iterator<int> to(args.num_samples);
    return seastar::do_for_each(from, to,
                                [&](int i) mutable {
                                  hist->record(dist(gen));
                                  return seastar::make_ready_future<>();
                                })
      .then([&] {
        seastar::prometheus::config conf;
        conf.metric_help = "synthetic histogram";
        conf.prefix = "histgen";
        return seastar::prometheus::add_prometheus_routes(http_server, conf)
          .then([&] {
            return http_server.listen(seastar::make_ipv4_address(
              args.ip.empty() ? seastar::ipv4_addr{args.port}
                              : seastar::ipv4_addr{args.ip, args.port}));
          });
      });
  }

  seastar::future<>
  stop() {
    return http_server.stop();
  }

  histgen_server_args args;
  seastar::lw_shared_ptr<smf::histogram> hist;
  seastar::http_server http_server;
  seastar::metrics::metric_groups metrics;
};

void
cli_opts(boost::program_options::options_description_easy_init o) {
  namespace po = boost::program_options;
  o("ip", po::value<std::string>()->default_value("127.0.0.1"),
    "ip to connect to");
  o("port", po::value<uint16_t>()->default_value(20777),
    "port for http stats service");
  o("min_val", po::value<uint64_t>()->default_value(0), "minimum sample value");
  o("max_val",
    po::value<uint64_t>()->default_value(smf::kDefaultHistogramMaxValue),
    "maximum sample value");
  o("num_samples", po::value<uint32_t>()->default_value(10000),
    "number of sample values");
}

int
main(int args, char **argv, char **env) {
  seastar::sharded<histgen_server> server;

  seastar::app_template app;
  cli_opts(app.add_options());

  return app.run_deprecated(args, argv, [&] {
    seastar::engine().at_exit([&] { return server.stop(); });

    auto &cfg = app.configuration();

    histgen_server_args args;
    args.ip = cfg["ip"].as<std::string>();
    args.port = cfg["port"].as<uint16_t>();
    args.min_val = cfg["min_val"].as<uint64_t>();
    args.max_val = cfg["max_val"].as<uint64_t>();
    args.num_samples = cfg["num_samples"].as<uint32_t>();

    return server.start(args).then(
      [&server] { return server.invoke_on_all(&histgen_server::serve); });
  });
}
