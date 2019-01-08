// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
// std
#include <chrono>
#include <iostream>
// seastar
#include <seastar/core/app-template.hh>
#include <seastar/core/distributed.hh>
// smf
#include "integration_tests/demo_service.smf.fb.h"
#include "integration_tests/non_root_port.h"
#include "smf/histogram_seastar_utils.h"
#include "smf/load_channel.h"
#include "smf/load_generator.h"
#include "smf/log.h"
#include "smf/random.h"
#include "smf/rpc_filter.h"
#include "smf/rpc_handle_router.h"
#include "smf/rpc_server.h"
#include "smf/unique_histogram_adder.h"
#include "smf/zstd_filter.h"

static const char *kPoem = "How do I love thee? Let me count the ways."
                           "I love thee to the depth and breadth and height"
                           "My soul can reach, when feeling out of sight"
                           "For the ends of being and ideal grace."
                           "I love thee to the level of every day's"
                           "Most quiet need, by sun and candle-light."
                           "I love thee freely, as men strive for right."
                           "I love thee purely, as they turn from praise."
                           "I love thee with the passion put to use"
                           "In my old griefs, and with my childhood's faith."
                           "I love thee with a love I seemed to lose"
                           "With my lost saints. I love thee with the breath,"
                           "Smiles, tears, of all my life; and, if God choose,"
                           "I shall but love thee better after death."
                           "How do I love thee? Let me count the ways."
                           "I love thee to the depth and breadth and height"
                           "My soul can reach, when feeling out of sight"
                           "For the ends of being and ideal grace."
                           "I love thee to the level of every day's"
                           "Most quiet need, by sun and candle-light."
                           "I love thee freely, as men strive for right."
                           "I love thee purely, as they turn from praise."
                           "I love thee with the passion put to use"
                           "In my old griefs, and with my childhood's faith."
                           "I love thee with a love I seemed to lose"
                           "With my lost saints. I love thee with the breath,"
                           "Smiles, tears, of all my life; and, if God choose,"
                           "I shall but love thee better after death.";

using client_t = smf_gen::demo::SmfStorageClient;
using load_gen_t = smf::load_generator<client_t>;

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
    req.data->name = kPoem;
    return req.serialize_data();
  }
};

class storage_service final : public smf_gen::demo::SmfStorage {
  virtual seastar::future<smf::rpc_typed_envelope<smf_gen::demo::Response>>
  Get(smf::rpc_recv_typed_context<smf_gen::demo::Request> &&rec) final {
    smf::rpc_typed_envelope<smf_gen::demo::Response> data;
    data.envelope.set_status(200);
    return seastar::make_ready_future<
      smf::rpc_typed_envelope<smf_gen::demo::Response>>(std::move(data));
  }
};

void
cli_opts(boost::program_options::options_description_easy_init o) {
  namespace po = boost::program_options;

  o("ip", po::value<std::string>()->default_value("127.0.0.1"),
    "ip to connect to");

  smf::random rand;
  o("port",
    po::value<uint16_t>()->default_value(
      smf::non_root_port(rand.next() % std::numeric_limits<uint16_t>::max())),
    "port for service");

  o("httpport",
    po::value<uint16_t>()->default_value(
      smf::non_root_port(rand.next() % std::numeric_limits<uint16_t>::max())),
    "port for http stats service");

  o("req-num", po::value<uint32_t>()->default_value(100),
    "number of request per concurrenct connection");

  o("concurrency", po::value<uint32_t>()->default_value(10),
    "number of green threads per real thread (seastar::futures<>)");
}

int
main(int args, char **argv, char **env) {
  LOG_INFO("About to start the RPC test");
  seastar::distributed<smf::rpc_server> rpc;
  seastar::distributed<load_gen_t> load;

  seastar::app_template app;

  cli_opts(app.add_options());

  return app.run(args, argv, [&]() -> seastar::future<int> {
#ifndef NDEBUG
    std::cout.setf(std::ios::unitbuf);
    smf::app_run_log_level(seastar::log_level::trace);
#endif
    LOG_INFO("Setting up at_exit hooks");
    seastar::engine().at_exit([&] { return load.stop(); });
    seastar::engine().at_exit([&] { return rpc.stop(); });

    auto &cfg = app.configuration();

    smf::rpc_server_args args;
    args.rpc_port = cfg["port"].as<uint16_t>();
    args.http_port = cfg["httpport"].as<uint16_t>();
    args.flags |= smf::rpc_server_flags::rpc_server_flags_disable_http_server;
    args.memory_avail_per_core =
      static_cast<uint64_t>(0.4 * seastar::memory::stats().total_memory());

    return rpc.start(args)
      .then([&rpc] {
        LOG_INFO("Registering smf_gen::demo::storage_service");
        return rpc.invoke_on_all(
          &smf::rpc_server::register_service<storage_service>);
      })
      .then([&rpc] {
        return rpc.invoke_on_all(&smf::rpc_server::register_incoming_filter<
                                 smf::zstd_decompression_filter>);
      })
      .then([&rpc] {
        LOG_INFO("Invoking rpc start on all cores");
        return rpc.invoke_on_all(&smf::rpc_server::start);
      })
      .then([&load, &cfg] {
        LOG_INFO("About to start the client");

        ::smf::load_generator_args largs(
          cfg["ip"].as<std::string>().c_str(), cfg["port"].as<uint16_t>(),
          cfg["req-num"].as<uint32_t>(), cfg["concurrency"].as<uint32_t>(),
          static_cast<uint64_t>(0.4 * seastar::memory::stats().total_memory()),
          smf::rpc::compression_flags::compression_flags_none, cfg);

        LOG_INFO("Load args: {}", largs);
        return load.start(std::move(largs));
      })
      .then([&load] {
        LOG_INFO("Connecting to server");
        return load.invoke_on_all(&load_gen_t::connect);
      })
      .then([&load] {
        LOG_INFO("Benchmarking server");
        return load.invoke_on_all([](load_gen_t &server) {
          load_gen_t::generator_cb_t gen = generator{};
          load_gen_t::method_cb_t method = method_callback{};
          return server.benchmark(gen, method).then([](auto test) {
            LOG_INFO("Test ran in:{}ms", test.duration_in_millis());
            return seastar::make_ready_future<>();
          });
        });
      })
      .then([&load] {
        LOG_INFO("MapReducing stats");
        return load
          .map_reduce(smf::unique_histogram_adder(),
                      [](load_gen_t &shard) { return shard.copy_histogram(); })
          .then([](auto h) {
            LOG_INFO("Writing client histograms");
            return smf::histogram_seastar_utils::write("clients_latency.hgrm",
                                                       std::move(h));
          });
      })
      .then([&] {
        return rpc
          .map_reduce(smf::unique_histogram_adder(),
                      &smf::rpc_server::copy_histogram)
          .then([](auto h) {
            LOG_INFO("Writing server histograms");
            return smf::histogram_seastar_utils::write("server_latency.hgrm",
                                                       std::move(h));
          });
      })
      .then([] {
        LOG_INFO("Exiting");
        return seastar::make_ready_future<int>(0);
      });
  });
}
