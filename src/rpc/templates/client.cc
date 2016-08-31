// std
#include <iostream>
#include <chrono>
#include <thread>
// seastar
#include <core/distributed.hh>
#include <core/app-template.hh>
// smf
#include "log.h"

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

// really just has it's own
// flatbuffers + rpc client - simple
struct requestor_channel {
  requestor_channel(const char *ip, uint16_t port)
    : client(new smf_gen::fbs::rpc::SmurfStorageClient(ipv4_addr{ip, port})) {
    client->enable_histogram_metrics();
    flatbuffers::FlatBufferBuilder fbb;
    auto req = smf_gen::fbs::rpc::CreateRequest(
      fbb, fbb.CreateString(kPayloadSonet43ElizabethBarretBowen));
    fbb.Finish(req);
    envelope = std::make_unique<smf::rpc_envelope>(fbb.GetBufferPointer(),
                                                   fbb.GetSize());
    // enable compression every time
    envelope->set_dynamic_compression_min_size(1000);
  }

  requestor_channel(const requestor_channel &) = delete;

  requestor_channel(requestor_channel &&o)
    : client(std::move(o.client)), envelope(std::move(o.envelope)) {}


  future<> connect() { return client->connect(); }
  future<> send_request(size_t reqs) {
    return client->Get(envelope.get())
      .then([this, reqs](auto r /* ignore reply*/) {
        if(reqs > 1) {
          return this->send_request(reqs - 1);
        } else {
          return make_ready_future<>();
        }
      });
  }
  std::unique_ptr<smf_gen::fbs::rpc::SmurfStorageClient> client;
  std::unique_ptr<smf::rpc_envelope> envelope;
};


class rpc_client_wrapper {
  public:
  rpc_client_wrapper(const char *ip,
                     uint16_t port,
                     size_t num_of_req,
                     size_t concurrency)
    : concurrency_(std::min(num_of_req, concurrency))
    , num_of_req_(num_of_req)
    , clients_(concurrency) {

    smf::LOG_INFO(
      "Setting up the client. Remove server: {}:{} , reqs:{}, concurrency:{} ",
      ip, port, num_of_req, concurrency);

    std::generate(clients_.begin(), clients_.end(), [ip, port] {
      return std::make_unique<requestor_channel>(ip, port);
    });
  }
  future<> connect() {
    smf::LOG_INFO("Creating connections");
    return do_for_each(clients_.begin(), clients_.end(),
                       [](auto &c) { return c->connect(); });
  }

  future<uint64_t> send_request() {
    using namespace std::chrono;
    auto begin = high_resolution_clock::now();
    return connect()
      .then([this] {
        return do_with(semaphore(concurrency_), [this](auto &limit) {
          return do_for_each(clients_.begin(), clients_.end(),
                             [this, &limit](auto &c) {
                               return limit.wait(1).then([this, &c, &limit] {
                                 c->send_request(num_of_req_ / concurrency_)
                                   .finally([&limit] { limit.signal(1); });
                               });
                             })
            .then([this, &limit] { return limit.wait(concurrency_); });
        }); // semaphore
      })
      .then([begin, this] {
        auto end_t = high_resolution_clock::now();
        uint64_t duration_milli =
          duration_cast<milliseconds>(end_t - begin).count();
        auto qps = double(num_of_req_) / std::max(duration_milli, uint64_t(1));
        smf::LOG_INFO("Test took: {}ms", duration_milli);
        smf::LOG_INFO("Queries per millisecond: {}/ms", qps);
        return make_ready_future<uint64_t>(duration_milli);
      }); // connect
  }

  future<> stop() {
    sstring s =
      "rpc_client_histogram_shard_" + to_sstring(engine().cpu_id()) + ".txt";
    FILE *fp = fopen(s.c_str(), "w");
    if(fp) {
      smf::histogram h;
      for(auto &ch : clients_) {
        h += *ch->client->get_histogram();
      }
      h.print(fp);
      fclose(fp);
    }
    return make_ready_future<>();
  }

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

int main(int args, char **argv, char **env) {
  smf::LOG_INFO("About to start the client");
  distributed<rpc_client_wrapper> clients;
  app_template app;
  app.add_options()("req_num", bpo::value<size_t>()->default_value(400),
                    "number of requests")(
    "concurrency", bpo::value<size_t>()->default_value(10),
    "number of concurrent requests")(
    "rpc_port", bpo::value<uint16_t>()->default_value(11225), "rpc port")(
    "ip_address", bpo::value<std::string>()->default_value("127.0.0.1"),
    "ip address of server");
  try {
    return app.run(args, argv, [&app, &clients]() -> future<int> {
      smf::log.set_level(seastar::log_level::debug);
      auto &&config = app.configuration();
      const uint16_t port = config["rpc_port"].as<uint16_t>();
      const size_t req_num = config["req_num"].as<size_t>();
      const size_t concurrency = config["concurrency"].as<size_t>();
      const char *ip = config["ip_address"].as<std::string>().c_str();
      smf::LOG_INFO("setting up exit hooks");
      engine().at_exit([&clients] { return clients.stop(); });
      return clients.start(ip, port, req_num, concurrency)
        .then([&clients] {
          return clients.map_reduce(adder<uint64_t>(),
                                    &rpc_client_wrapper::send_request)
            .then([](auto ms) {
              smf::LOG_INFO("Total time for all requests: {}ms", ms);
              return make_ready_future<>();
            });
        })
        .then([&clients] {
          smf::LOG_INFO("Shutting down clients");
          return clients.stop().then([] { return make_ready_future<int>(0); });

        });
    });
  } catch(const std::exception &e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
  }
}
