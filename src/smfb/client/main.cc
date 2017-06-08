// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#include <iostream>
#include <limits>

#include <core/app-template.hh>

#include "chain_replication/chain_replication_service.h"
#include "hashing/hashing_utils.h"
#include "histogram/histogram_seastar_utils.h"
#include "rpc/load_gen/load_channel.h"
#include "rpc/load_gen/load_generator.h"
#include "utils/random.h"
#include "utils/time_utils.h"

using client_t   = smf::chains::chain_replication_client;
using load_gen_t = smf::load_gen::load_generator<client_t>;


struct test_put {
  seastar::future<> operator()(client_t *c, smf::rpc_envelope &&e) {
    return c->put(std::move(e)).then([](auto ret) {
      return seastar::make_ready_future<>();
    });
  }
};

struct put_data_generator {
  smf::random &random() {
    static thread_local smf::random rand;
    return rand;
  }

  smf::rpc_envelope operator()(
    const boost::program_options::variables_map &cfg) {
    // anti pattern w/ seastar, but boost ... has no conversion to
    // seastar::sstring

    std::string topic = cfg["topic"].as<std::string>();
    std::string key   = cfg["key"].as<std::string>();
    std::string value = cfg["value"].as<std::string>();
    uint32_t    max_rand_bytes =
      static_cast<uint32_t>(std::numeric_limits<uint16_t>::max());

    if (key == "random") {
      key = random().next_str(random().next() % max_rand_bytes).c_str();
    }
    if (value == "random") {
      value = random().next_str(random().next() % max_rand_bytes).c_str();
    }
    if (topic == "random") {
      topic = random().next_str(random().next() % max_rand_bytes).c_str();
    }

    smf::chains::tx_put_requestT native_req;
    native_req.topic     = topic;
    native_req.partition = static_cast<uint32_t>(random().next());
    native_req.chain.push_back(uint32_t(2130706433) /*127.0.0.1*/);

    for (auto i = 0u, max = cfg["batch-size"].as<uint32_t>(); i < max; ++i) {
      auto frag         = std::make_unique<smf::chains::tx_fragmentT>();
      frag->op          = smf::chains::tx_operation::tx_operation_full;
      frag->id          = static_cast<uint32_t>(random().next());
      frag->time_micros = smf::time_now_micros();

      frag->key.reserve(key.size());
      frag->value.reserve(value.size());

      std::copy(key.begin(), key.end(), std::back_inserter(frag->key));
      std::copy(value.begin(), value.end(), std::back_inserter(frag->key));

      native_req.txs.push_back(std::move(frag));
    }
    return smf::rpc_envelope(
      smf::rpc_letter::native_table_to_rpc_letter<smf::chains::tx_put_request>(
        native_req));
  }
};

void cli_opts(boost::program_options::options_description_easy_init o) {
  namespace po = boost::program_options;

  o("ip", po::value<std::string>()->default_value("127.0.0.1"),
    "ip to connect to");

  o("port", po::value<uint16_t>()->default_value(11201), "port for service");

  o("req-num", po::value<uint32_t>()->default_value(1000),
    "number of request per concurrenct connection");

  o("concurrency", po::value<uint32_t>()->default_value(10),
    "number of green threads per real thread (seastar::futures<>)");

  o("topic", po::value<std::string>()->default_value("dummy_topic"),
    "client in which to enqueue records, set to `random' to auto gen");

  o("key", po::value<std::string>()->default_value("dummy_key"),
    "key to enqueue on broker, set to `random' to auto gen");

  o("value", po::value<std::string>()->default_value("dummy_value"),
    "value to enqueue on broker, set to `random' to auto gen");

  o("batch-size", po::value<uint32_t>()->default_value(1000),
    "number of request per concurrenct connection");
}


int main(int argc, char **argv, char **env) {
  seastar::distributed<load_gen_t> load;
  seastar::app_template            app;

  try {
    cli_opts(app.add_options());
    return app.run(argc, argv, [&] {
      seastar::engine().at_exit([&load] { return load.stop(); });

      auto &cfg = app.configuration();

      ::smf::load_gen::generator_args largs(
        cfg["ip"].as<std::string>().c_str(), cfg["port"].as<uint16_t>(),
        cfg["req-num"].as<uint32_t>(), cfg["concurrency"].as<uint32_t>(), cfg);

      LOG_INFO("Preparing load of test: {}", largs);

      return load.start(std::move(largs))
        .then([&load] {
          LOG_INFO("Connecting to server");
          return load.invoke_on_all(&load_gen_t::connect);
        })
        .then([&load] {
          LOG_INFO("Benchmarking server");
          return load.invoke_on_all([](load_gen_t &server) {
            load_gen_t::method_cb_t    method = test_put{};
            load_gen_t::generator_cb_t gen    = put_data_generator{};

            return server.benchmark(gen, method).then([](auto test) {
              LOG_INFO("Test ran in:{}ms", test.duration_in_millis());
              return seastar::make_ready_future<>();
            });
          });
        })
        .then([&load] {
          LOG_INFO("Requesting load histogram from all cores");
          return load
            .map_reduce(
              seastar::adder<smf::histogram>(),
              [](load_gen_t &shard) { return shard.copy_histogram(); })
            .then([](smf::histogram h) {
              LOG_INFO("Writing histogram: load_hdr.hgrm");
              return smf::histogram_seastar_utils::write_histogram(
                "load_hdr.hgrm", std::move(h));
            });
        })
        .then([] {
          LOG_INFO("End load");
          return seastar::make_ready_future<int>(0);
        });
    });
  } catch (...) {
    LOG_INFO("Exception while running: {}", std::current_exception());
    return 1;
  }
  return 0;
}
