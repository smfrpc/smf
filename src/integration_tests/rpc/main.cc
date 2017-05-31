// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
// std
#include <chrono>
#include <iostream>
// seastar
#include <core/app-template.hh>
#include <core/distributed.hh>
// smf
#include "histogram/histogram_seastar_utils.h"
#include "platform/log.h"
#include "rpc/filters/zstd_filter.h"
#include "rpc/load_gen/load_channel.h"
#include "rpc/load_gen/load_generator.h"
#include "rpc/rpc_filter.h"
#include "rpc/rpc_handle_router.h"
#include "rpc/rpc_server.h"
#include "rpc/rpc_server_stats_printer.h"

// templates
#include "flatbuffers/demo_service.smf.fb.h"

using client_t   = smf_gen::demo::SmfStorageClient;
using load_gen_t = smf::load_gen::load_generator<client_t>;

struct method_callback {
  seastar::future<> operator()(client_t *c, smf::rpc_envelope &&e) {
    return c->Get(std::move(e)).then([](auto ret) {
      return seastar::make_ready_future<>();
    });
  }
};

struct generator {
  smf::rpc_envelope operator()(
    const boost::program_options::variables_map &cfg) {
    auto req = smf_gen::demo::RequestT{};
    req.name = "How do I love thee? Let me count the ways."
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

    return smf::rpc_envelope(
      smf::rpc_letter::native_table_to_rpc_letter<smf_gen::demo::Request>(req));
  }
};


class storage_service : public smf_gen::demo::SmfStorage {
  virtual seastar::future<smf::rpc_typed_envelope<smf_gen::demo::Response>> Get(
    smf::rpc_recv_typed_context<smf_gen::demo::Request> &&rec) final {
    smf::rpc_typed_envelope<smf_gen::demo::Response> data;
    data.envelope.set_status(200);
    return seastar::
      make_ready_future<smf::rpc_typed_envelope<smf_gen::demo::Response>>(
        std::move(data));
  }
};


void cli_opts(boost::program_options::options_description_easy_init o) {
  namespace po = boost::program_options;

  o("ip", po::value<std::string>()->default_value("127.0.0.1"),
    "ip to connect to");

  o("port", po::value<uint16_t>()->default_value(11225), "port for service");

  o("req-num", po::value<uint32_t>()->default_value(100),
    "number of request per concurrenct connection");

  o("concurrency", po::value<uint32_t>()->default_value(10),
    "number of green threads per real thread (seastar::futures<>)");
}


int main(int args, char **argv, char **env) {
  // SET_LOG_LEVEL(seastar::log_level::debug);
  DLOG_DEBUG("About to start the RPC test");
  seastar::distributed<smf::rpc_server_stats> stats;
  seastar::distributed<smf::rpc_server>       rpc;
  seastar::distributed<load_gen_t>            load;
  smf::rpc_server_stats_printer               stats_printer(&stats);

  seastar::app_template app;

  cli_opts(app.add_options());

  return app.run(args, argv, [&]() -> seastar::future<int> {
    DLOG_DEBUG("Setting up at_exit hooks");
    seastar::engine().at_exit([&] { return stats_printer.stop(); });
    seastar::engine().at_exit([&] { return stats.stop(); });
    seastar::engine().at_exit([&] { return rpc.stop(); });
    seastar::engine().at_exit([&] { return load.stop(); });

    auto &cfg = app.configuration();

    DLOG_DEBUG("starting stats");
    return stats.start()
      .then([&stats_printer] {
        DLOG_DEBUG("Starting printer");
        stats_printer.start();
        return seastar::make_ready_future<>();
      })
      .then([&rpc, &stats, &cfg] {
        uint32_t flags = smf::rpc_server_flags::rpc_server_flags_none;
        uint16_t port  = cfg["port"].as<uint16_t>();
        return rpc.start(&stats, port, flags).then([&rpc] {
          DLOG_DEBUG("Registering smf_gen::demo::storage_service");
          return rpc.invoke_on_all(
            &smf::rpc_server::register_service<storage_service>);
        });
      })
      .then([&rpc] {
        return rpc.invoke_on_all(
          &smf::rpc_server::
            register_incoming_filter<smf::zstd_decompression_filter>);
      })
      .then([&rpc] {
        DLOG_DEBUG("Invoking rpc start on all cores");
        return rpc.invoke_on_all(&smf::rpc_server::start);
      })
      .then([&load, &cfg] {
        DLOG_DEBUG("About to start the client");

        ::smf::load_gen::generator_args largs(
          cfg["ip"].as<std::string>().c_str(), cfg["port"].as<uint16_t>(),
          cfg["req-num"].as<uint32_t>(), cfg["concurrency"].as<uint32_t>(),
          cfg);
        DLOG_DEBUG("Load args: {}", largs);
        return load.start(std::move(largs));
      })
      .then([&load] {
        DLOG_DEBUG("Connecting to server");
        return load.invoke_on_all(&load_gen_t::connect);
      })
      .then([&load] {
        DLOG_DEBUG("Benchmarking server");
        return load.invoke_on_all([](load_gen_t &server) {
          load_gen_t::generator_cb_t gen    = generator{};
          load_gen_t::method_cb_t    method = method_callback{};
          return server.benchmark(gen, method).then([](auto test) {
            DLOG_DEBUG("Test ran in:{}ms", test.duration_in_millis());
            return seastar::make_ready_future<>();
          });
        });
      })
      .then([&load] {
        DLOG_DEBUG("MapReducing stats");
        return load
          .map_reduce(seastar::adder<smf::histogram>(),
                      [](load_gen_t &shard) { return shard.copy_histogram(); })
          .then([](smf::histogram h) {
            DLOG_DEBUG("Writing client histograms");
            return smf::histogram_seastar_utils::write_histogram(
              "clients_hdr.txt", std::move(h));
          });
      })
      .then([&] {
        return rpc
          .map_reduce(seastar::adder<smf::histogram>(),
                      &smf::rpc_server::copy_histogram)
          .then([](smf::histogram h) {
            DLOG_DEBUG("Writing server histograms");
            return smf::histogram_seastar_utils::write_histogram(
              "server_hdr.txt", std::move(h));
          });
      })
      .then([] {
        DLOG_DEBUG("Exiting");
        return seastar::make_ready_future<int>(0);
      });
  });
}
