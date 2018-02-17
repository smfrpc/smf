// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#include <iostream>
#include <limits>

#include <core/app-template.hh>
#include <fastrange/fastrange.h>

#include "flatbuffers/wal_generated.h"

#include "chain_replication/chain_replication_service.h"
#include "filesystem/wal_write_projection.h"
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
  seastar::future<>
  operator()(client_t *c, smf::rpc_envelope &&e) {
    return c->put(std::move(e)).then([](auto ret) {
      return seastar::make_ready_future<>();
    });
  }
};

struct put_data_generator {
  static constexpr std::size_t kMaxPartitions = 32;
  static constexpr uint32_t    kMaxRandBytes =
    static_cast<uint32_t>(std::numeric_limits<uint8_t>::max());


  smf::random &
  random() {
    static thread_local smf::random rand;
    return rand;
  }

  std::vector<std::unique_ptr<smf::wal::tx_put_partition_tupleT>> &
  frags() {
    static thread_local std::vector<
      std::unique_ptr<smf::wal::tx_put_partition_tupleT>>
      lvec(kMaxPartitions);
    return lvec;
  }

  const seastar::sstring &
  key(const boost::program_options::variables_map &cfg) {
    static thread_local seastar::sstring key = [this, &cfg]() {
      auto s = cfg["key"].as<seastar::sstring>();
      if (s == "random") {
        s = random().next_str(random().next() % kMaxRandBytes);
      }
      return s;
    }();

    return key;
  }
  const seastar::sstring &
  topic(const boost::program_options::variables_map &cfg) {
    static thread_local seastar::sstring key = [this, &cfg]() {
      auto s = cfg["topic"].as<seastar::sstring>();
      if (s == "random") {
        s = random().next_str(random().next() % kMaxRandBytes);
      }
      return s;
    }();

    return key;
  }
  const seastar::sstring &
  value(const boost::program_options::variables_map &cfg) {
    static thread_local seastar::sstring key = [this, &cfg]() {
      auto s = cfg["value"].as<seastar::sstring>();
      if (s == "random") {
        s = random().next_str(random().next() % kMaxRandBytes);
      }
      return s;
    }();
    return key;
  }


  smf::rpc_envelope
  operator()(const boost::program_options::variables_map &cfg) {
    smf::rpc_typed_envelope<smf::chains::chain_put_request> typed_env;
    auto &p       = *typed_env.data.get();
    p.chain_index = 0;
    p.chain.push_back(uint32_t(2130706433) /*127.0.0.1*/);
    p.put        = std::make_unique<smf::wal::tx_put_requestT>();
    p.put->topic = topic(cfg);

    const uint32_t batch_size = cfg["batch-size"].as<uint32_t>();
    auto           partition =
      fastrange32(static_cast<uint32_t>(random().next()), kMaxPartitions);

    if (frags()[partition] == nullptr) {
      auto ptr       = std::make_unique<smf::wal::tx_put_partition_tupleT>();
      ptr->partition = partition;
      for (auto j = 0u; j < batch_size; ++j) {
        smf::wal::tx_put_fragmentT frag;
        frag.op       = smf::wal::tx_put_operation::tx_put_operation_full;
        frag.epoch_ms = smf::lowres_time_now_millis();
        frag.type     = smf::wal::tx_put_fragment_type::tx_put_fragment_type_kv;
        frag.key.resize(key(cfg).size(), 0);
        frag.value.resize(value(cfg).size(), 0);
        std::memcpy(&frag.key[0], key(cfg).data(), key(cfg).size());
        std::memcpy(&frag.value[0], value(cfg).data(), value(cfg).size());
        ptr->txs.emplace_back(smf::wal_write_projection::xform(frag));
      }
      frags()[partition] = std::move(ptr);
    }
    // copy
    auto ptr       = std::make_unique<smf::wal::tx_put_partition_tupleT>();
    ptr->partition = partition;
    for (auto &bf : frags()[partition]->txs) {
      auto bin = std::make_unique<smf::wal::tx_put_binary_fragmentT>();
      bin->data.resize(bf->data.size());
      std::memcpy(bin->data.data(), bf->data.data(), bf->data.size());
      ptr->txs.push_back(std::move(bin));
    }
    // return yet one last copy! yikes
    p.put->data.push_back(std::move(ptr));
    return typed_env.serialize_data();
  }
};

void
cli_opts(boost::program_options::options_description_easy_init o) {
  namespace po = boost::program_options;

  o("ip", po::value<seastar::sstring>()->default_value("127.0.0.1"),
    "ip to connect to");

  o("port", po::value<uint16_t>()->default_value(11201), "port for service");

  o("req-num", po::value<uint32_t>()->default_value(1000),
    "number of request per concurrenct connection");

  o("concurrency", po::value<uint32_t>()->default_value(3),
    "number of green threads per real thread (seastar::futures<>)");

  o("topic", po::value<seastar::sstring>()->default_value("dummy_topic"),
    "client in which to enqueue records, set to `random' to auto gen");

  o("key", po::value<seastar::sstring>()->default_value("dummy_key"),
    "key to enqueue on broker, set to `random' to auto gen");

  o("value", po::value<seastar::sstring>()->default_value("dummy_value"),
    "value to enqueue on broker, set to `random' to auto gen");

  o("batch-size", po::value<uint32_t>()->default_value(1),
    "number of request per concurrenct connection");
}


int
main(int argc, char **argv, char **env) {
  std::setvbuf(stdout, nullptr, _IOLBF, 1024);
  seastar::distributed<load_gen_t> load;
  seastar::app_template            app;

  try {
    cli_opts(app.add_options());
    return app.run(argc, argv, [&] {
      seastar::engine().at_exit([&load] { return load.stop(); });

      auto &cfg = app.configuration();

      ::smf::load_gen::generator_args largs(
        cfg["ip"].as<seastar::sstring>().c_str(), cfg["port"].as<uint16_t>(),
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

            return server.benchmark(gen, method)
              .then([](smf::load_gen::generator_duration test) {
                LOG_INFO("Test {}", test);
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
