// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#include <iostream>
#include <limits>

#include <core/app-template.hh>
#include <fastrange.h>

#include "chain_replication/chain_replication_service.h"
#include "flatbuffers/chain_replication.smf.fb.h"
#include "hashing/hashing_utils.h"
#include "histogram/histogram_seastar_utils.h"
#include "histogram/unique_histogram_adder.h"
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
      static_cast<uint32_t>(std::numeric_limits<uint8_t>::max());

    if (key == "random") {
      key = random().next_str(random().next() % max_rand_bytes).c_str();
    }
    if (value == "random") {
      value = random().next_str(random().next() % max_rand_bytes).c_str();
    }
    if (topic == "random") {
      topic = random().next_str(random().next() % max_rand_bytes).c_str();
    }

    smf::rpc_typed_envelope<smf::chains::chain_put_request> typed_env;
    auto &p       = *typed_env.data.get();
    p.chain_index = 0;
    p.chain.push_back(uint32_t(2130706433) /*127.0.0.1*/);
    p.put        = std::make_unique<smf::wal::tx_put_requestT>();
    p.put->topic = topic;

    const uint32_t batch_size = cfg["batch-size"].as<uint32_t>();
    const uint64_t fragment_count =
      std::max(uint64_t(1), random().next() % batch_size);
    const uint64_t puts_per_partition =
      std::max(uint64_t(1),
               static_cast<uint64_t>(std::ceil(batch_size / fragment_count)));

    for (auto i = 0u; i < puts_per_partition; ++i) {
      auto ptr       = std::make_unique<smf::wal::tx_put_partition_pairT>();
      ptr->partition = fastrange32(static_cast<uint32_t>(random().next()), 32);

      for (auto j = 0u; j < fragment_count; ++j) {
        auto f = std::make_unique<smf::wal::tx_put_fragmentT>();
        // data + commit
        f->op       = smf::wal::tx_put_operation::tx_put_operation_full;
        f->epoch_ms = smf::lowres_time_now_millis();
        f->type     = smf::wal::tx_put_fragment_type::tx_put_fragment_type_kv;
        f->key.resize(key.size(), 0);
        f->value.resize(value.size(), 0);
        std::memcpy(&f->key[0], &key[0], key.size());
        std::memcpy(&f->value[0], &value[0], value.size());
        ptr->txs.push_back(std::move(f));
      }
      p.put->data.push_back(std::move(ptr));
    }
    return typed_env.serialize_data();
  }
};

void cli_opts(boost::program_options::options_description_easy_init o) {
  namespace po = boost::program_options;

  o("ip", po::value<std::string>()->default_value("127.0.0.1"),
    "ip to connect to");

  o("port", po::value<uint16_t>()->default_value(11201), "port for service");

  o("req-num", po::value<uint32_t>()->default_value(1000),
    "number of request per concurrenct connection");

  o("concurrency", po::value<uint32_t>()->default_value(3),
    "number of green threads per real thread (seastar::futures<>)");

  o("topic", po::value<std::string>()->default_value("dummy_topic"),
    "client in which to enqueue records, set to `random' to auto gen");

  o("key", po::value<std::string>()->default_value("dummy_key"),
    "key to enqueue on broker, set to `random' to auto gen");

  o("value", po::value<std::string>()->default_value("dummy_value"),
    "value to enqueue on broker, set to `random' to auto gen");

  o("batch-size", po::value<uint32_t>()->default_value(1),
    "number of request per concurrenct connection");
}


int main(int argc, char **argv, char **env) {
  std::setvbuf(stdout, nullptr, _IOLBF, 1024);
  seastar::distributed<load_gen_t> load;
  seastar::app_template            app;

  try {
    cli_opts(app.add_options());
    return app.run(argc, argv, [&] {
      seastar::engine().at_exit([&load] { return load.stop(); });

      auto &cfg = app.configuration();

      ::smf::load_gen::generator_args largs(
        cfg["ip"].as<std::string>().c_str(), cfg["port"].as<uint16_t>(),
        cfg["req-num"].as<uint32_t>(), cfg["concurrency"].as<uint32_t>(),
        static_cast<uint64_t>(0.9 * seastar::memory::stats().total_memory()),
        smf::rpc::compression_flags::compression_flags_lz4, cfg);

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
              LOG_INFO("Test {}", test.as_sstring());
              return seastar::make_ready_future<>();
            });
          });
        })
        .then([&load] {
          LOG_INFO("Requesting load histogram from all cores");
          return load
            .map_reduce(
              smf::unique_histogram_adder(),
              [](load_gen_t &shard) { return shard.copy_histogram(); })
            .then([](auto h) {
              LOG_INFO("Writing histogram: load_hdr.hgrm");
              return smf::histogram_seastar_utils::write("load_hdr.hgrm",
                                                         std::move(h));
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
