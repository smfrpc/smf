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
#include "log.h"
#include "rpc/filters/zstd_filter.h"
#include "rpc/rpc_filter.h"
#include "rpc/rpc_handle_router.h"
#include "rpc/rpc_server.h"
#include "rpc/rpc_server_stats_printer.h"
// templates
#include "rpc/smf_gen/demo_service.smf.fb.h"

namespace bpo = boost::program_options;
static const char *kPayloadSonet43ElizabethBarretBowen =
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


struct requestor_channel {
  requestor_channel(const char *ip, uint16_t port)
    : client(new smf_gen::fbs::rpc::SmfStorageClient(ipv4_addr{ip, port}))
    , fbb(new flatbuffers::FlatBufferBuilder()) {
    client->enable_histogram_metrics();
    auto req = smf_gen::fbs::rpc::CreateRequest(
      *fbb.get(), fbb->CreateString(kPayloadSonet43ElizabethBarretBowen));
    fbb->Finish(req);
    client->register_outgoing_filter(smf::zstd_compression_filter(1000));
  }
  requestor_channel(const requestor_channel &) = delete;
  requestor_channel(requestor_channel &&o)
    : client(std::move(o.client)), fbb(std::move(o.fbb)) {}
  future<> connect() { return client->connect(); }
  future<> send_request(size_t reqs) {
    return do_for_each(boost::counting_iterator<int>(0),
                       boost::counting_iterator<int>(reqs), [this](int) {
                         smf::rpc_envelope e(fbb->GetBufferPointer(),
                                             fbb->GetSize());
                         return client->Get(std::move(e)).then([](auto t) {
                           return make_ready_future<>();
                         });
                       });
  }
  std::unique_ptr<smf_gen::fbs::rpc::SmfStorageClient> client;
  std::unique_ptr<flatbuffers::FlatBufferBuilder>      fbb;
};

class rpc_client_wrapper {
 public:
  rpc_client_wrapper(const char *ip,
                     uint16_t    port,
                     size_t      num_of_req,
                     size_t      concurrency)
    : concurrency_(std::min(num_of_req, concurrency))
    , num_of_req_(num_of_req)
    , clients_(concurrency) {
    DLOG_DEBUG(
      "Setting up the client. Remove server: {}:{} , reqs:{}, concurrency:{} ",
      ip, port, num_of_req, concurrency);

    std::generate(clients_.begin(), clients_.end(), [ip, port] {
      return std::make_unique<requestor_channel>(ip, port);
    });
  }
  future<> connect() {
    DLOG_DEBUG("Creating connections");
    return do_for_each(clients_.begin(), clients_.end(),
                       [](auto &c) { return c->connect(); });
  }

  future<uint64_t> send_request() {
    namespace co = std::chrono;
    auto begin   = co::high_resolution_clock::now();
    return connect()
      .then([this] {
        return do_with(semaphore(concurrency_), [this](auto &limit) {
          return do_for_each(clients_.begin(), clients_.end(),
                             [this, &limit](auto &c) {
                               return limit.wait(1).then([this, &c, &limit] {
                                 // notice that this does not return, hence
                                 // executing concurrently
                                 c->send_request(num_of_req_ / concurrency_)
                                   .finally([&limit] { limit.signal(1); });
                               });
                             })
            .then([this, &limit] { return limit.wait(concurrency_); });
        });  // semaphore
      })
      .then([begin, this] {
        auto     end_t = co::high_resolution_clock::now();
        uint64_t duration_milli =
          co::duration_cast<co::milliseconds>(end_t - begin).count();
        auto qps = static_cast<double>(num_of_req_)
                   / static_cast<double>(std::max(duration_milli, uint64_t(1)));
        DLOG_DEBUG("Test took: {}ms", duration_milli);
        DLOG_DEBUG("Queries per millisecond: {}/ms", qps);
        return make_ready_future<uint64_t>(duration_milli);
      });  // connect
  }

  future<smf::histogram> all_histograms() {
    smf::histogram h;
    for (auto &ch : clients_) {
      h += *ch->client->get_histogram();
    }
    return make_ready_future<smf::histogram>(std::move(h));
  }


  future<> stop() { return make_ready_future<>(); }

 private:
  const size_t concurrency_;
  const size_t num_of_req_;
  // You can only have one client per active stream
  // the problem comes when you try to read, 2 function calls to read, even
  // read_exactly might interpolate
  // which yields incorrect results for test. That use case has to be manually
  // optimized and don't expect it to be the main use case
  // In effect, you need a socket per concurrency item in the command line
  // flags
  //
  std::vector<std::unique_ptr<requestor_channel>> clients_{};
};


class storage_service : public smf_gen::fbs::rpc::SmfStorage {
  future<smf::rpc_envelope> Get(
    smf::rpc_recv_typed_context<smf_gen::fbs::rpc::Request> &&rec) final {
    // DLOG_DEBUG("Server got payload {}", rec.get()->name()->size());
    smf::rpc_envelope e(nullptr);
    e.set_status(200);
    return make_ready_future<smf::rpc_envelope>(std::move(e));
  }
};


int main(int args, char **argv, char **env) {
  // SET_LOG_LEVEL(seastar::log_level::debug);
  DLOG_DEBUG("About to start the RPC test");
  distributed<smf::rpc_server_stats> stats;
  distributed<smf::rpc_server>       rpc;
  distributed<rpc_client_wrapper>    clients;
  smf::rpc_server_stats_printer      stats_printer(&stats);
  app_template                       app;
  app.add_options()("rpc_port", bpo::value<uint16_t>()->default_value(11225),
                    "rpc port")(
    "req_num", bpo::value<size_t>()->default_value(1000), "number of requests")(
    "concurrency", bpo::value<size_t>()->default_value(10),
    "number of concurrent requests")(
    "ip_address", bpo::value<std::string>()->default_value("127.0.0.1"),
    "ip address of server");

  return app.run(args, argv, [&app, &stats, &rpc, &clients,
                              &stats_printer]() -> future<int> {
    DLOG_DEBUG("Setting up at_exit hooks");

    engine().at_exit([&] { return stats_printer.stop(); });
    engine().at_exit([&] { return stats.stop(); });
    engine().at_exit([&] { return rpc.stop(); });
    engine().at_exit([&] { return clients.stop(); });

    auto &&  config = app.configuration();
    uint16_t port   = config["rpc_port"].as<uint16_t>();
    DLOG_DEBUG("starting stats");
    return stats.start()
      .then([&stats_printer] {
        DLOG_DEBUG("Starting stats");
        stats_printer.start();
        return make_ready_future<>();
      })
      .then([&rpc, &stats, port] {
        uint32_t flags = smf::RPCFLAGS::RPCFLAGS_LOAD_SHEDDING_ON;
        return rpc.start(&stats, port, flags).then([&rpc] {
          DLOG_DEBUG("Registering smf_gen::fbs::rpc::storage_service");
          return rpc.invoke_on_all(
            &smf::rpc_server::register_service<storage_service>);
        });
      })
      .then([&rpc] {
        /// example using a struct template
        return rpc.invoke_on_all(&smf::rpc_server::register_incoming_filter,
                                 smf::zstd_decompression_filter());
      })
      .then([&rpc] {
        DLOG_DEBUG("Invoking rpc start on all cores");
        return rpc.invoke_on_all(&smf::rpc_server::start);
      })
      .then([&clients, &config] {
        DLOG_DEBUG("About to start the client");
        const uint16_t port        = config["rpc_port"].as<uint16_t>();
        const size_t   req_num     = config["req_num"].as<size_t>();
        const size_t   concurrency = config["concurrency"].as<size_t>();
        const char *   ip = config["ip_address"].as<std::string>().c_str();
        return clients.start(ip, port, req_num, concurrency);
      })
      .then([&clients] {
        return clients
          .map_reduce(adder<uint64_t>(), &rpc_client_wrapper::send_request)
          .then([](auto ms) {
            DLOG_DEBUG("Total time for all requests: {}ms", ms);
            return make_ready_future<>();
          });
      })
      .then([&clients] {
        DLOG_DEBUG("MapReducing stats");
        return clients
          .map_reduce(adder<smf::histogram>(),
                      &rpc_client_wrapper::all_histograms)
          .then([](smf::histogram h) {
            DLOG_DEBUG("Writing client histograms");
            return smf::histogram_seastar_utils::write_histogram(
              "clients_hdr.txt", std::move(h));
          });
      })
      .then([&] {
        return rpc
          .map_reduce(adder<smf::histogram>(), &smf::rpc_server::copy_histogram)
          .then([](smf::histogram h) {
            DLOG_DEBUG("Writing server histograms");
            return smf::histogram_seastar_utils::write_histogram(
              "server_hdr.txt", std::move(h));
          });
      })
      .then([] {
        DLOG_DEBUG("Exiting");
        return make_ready_future<int>(0);
      });
  });
}
