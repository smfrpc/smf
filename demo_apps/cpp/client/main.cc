// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//

#include <chrono>
#include <iostream>

#include <seastar/core/app-template.hh>
#include <seastar/core/distributed.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/thread.hh>

#include "smf/histogram_seastar_utils.h"
#include "smf/load_channel.h"
#include "smf/load_generator.h"
#include "smf/log.h"
#include "smf/unique_histogram_adder.h"

// templates
#include "demo_service.smf.fb.h"

using client_t = smf_gen::demo::SmfStorageClient;
using load_gen_t = smf::load_generator<client_t>;
static const char *kPayload1Kbytes =
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

// This is just for the load generation.
// On normal apps you would just do
// client-><method_name>(params).then().then()
// as you would for normal seastar calls
// This example is just using the load generator to test performance
//
struct method_callback {
  seastar::future<>
  operator()(client_t *c, smf::rpc_envelope &&e) {
    return c->Get(std::move(e)).then([](auto ret) {
      return seastar::make_ready_future<>();
    });
  }
};

struct generator {
  smf::rpc_envelope
  operator()(const boost::program_options::variables_map &cfg) {
    smf::rpc_typed_envelope<smf_gen::demo::Request> req;
    req.data->name = kPayload1Kbytes;
    return req.serialize_data();
  }
};

void
cli_opts(boost::program_options::options_description_easy_init o) {
  namespace po = boost::program_options;

  o("ip", po::value<std::string>()->default_value("127.0.0.1"),
    "ip to connect to");

  o("port", po::value<uint16_t>()->default_value(20776), "port for service");

  o("req-num", po::value<uint32_t>()->default_value(1000),
    "number of request per concurrenct connection");

  // currently these are sockets
  o("concurrency", po::value<uint32_t>()->default_value(10),
    "number of green threads per real thread (seastar::futures<>)");

  o("ca-cert", po::value<std::string>()->default_value(""),
    "CA root certificate");
}

int
main(int args, char **argv, char **env) {
  seastar::distributed<load_gen_t> load;
  seastar::app_template app;
  cli_opts(app.add_options());

  return app.run(args, argv, [&] {
    seastar::engine().at_exit([&] { return load.stop(); });
    auto &cfg = app.configuration();

    return seastar::async([&] {
      ::smf::load_generator_args largs(
        cfg["ip"].as<std::string>().c_str(), cfg["port"].as<uint16_t>(),
        cfg["req-num"].as<uint32_t>(), cfg["concurrency"].as<uint32_t>(),
        static_cast<uint64_t>(0.9 * seastar::memory::stats().total_memory()),
        smf::rpc::compression_flags::compression_flags_none, cfg);

      // TODO(lumontec): uniform largs instantiation with server side
      auto ca_cert = cfg["ca-cert"].as<std::string>();
      if (ca_cert != "") {
        auto builder = seastar::tls::credentials_builder();
        builder.set_x509_trust_file(ca_cert, seastar::tls::x509_crt_format::PEM)
          .get0();
        largs.credentials =
          builder.build_reloadable_certificate_credentials().get0();
      }

      LOG_INFO("Load args: {}", largs);
      load.start(std::move(largs)).get();

      LOG_INFO("Connecting to server");
      load.invoke_on_all(&load_gen_t::connect).get();

      LOG_INFO("Benchmarking server");
      load
        .invoke_on_all([](load_gen_t &server) {
          load_gen_t::generator_cb_t gen = generator{};
          load_gen_t::method_cb_t method = method_callback{};
          return server.benchmark(gen, method).then([](auto test) {
            LOG_INFO("Bench: {}", test);
            return seastar::make_ready_future<>();
          });
        })
        .get();

      LOG_INFO("MapReducing stats");
      load
        .map_reduce(smf::unique_histogram_adder(),
                    [](load_gen_t &shard) { return shard.copy_histogram(); })
        .then([](std::unique_ptr<smf::histogram> h) {
          LOG_INFO("Writing client histograms");
          return smf::histogram_seastar_utils::write("clients_latency.hgrm",
                                                     std::move(h));
        })
        .get();

      LOG_INFO("Exiting");
      seastar::make_ready_future<int>(0).get();
    });
  });
}
