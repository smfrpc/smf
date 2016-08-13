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


class rpc_client_wrapper {
  public:
  rpc_client_wrapper(const char *ip, uint16_t port) {
    smf::LOG_INFO("Setting up the client. Remove server: {}:{}", ip, port);
    client_ = std::make_unique<smf_gen::fbs::rpc::SmurfStorageClient>(
      ipv4_addr{ip, port});
    client_->enable_histogram_metrics();
    auto req = smf_gen::fbs::rpc::CreateRequest(builder_,
                                                builder_.CreateString("hello"));
    builder_.Finish(req);
    envelope_ = std::make_unique<smf::rpc_envelope>(builder_.GetBufferPointer(),
                                                    builder_.GetSize());
  }

  future<> send_request() {
    return client_->Get(envelope_.get())
      .then([](auto r /* ignore reply*/) { return make_ready_future<>(); });
  }

  future<uint64_t> do_req(const size_t num_of_req) {
    using namespace std::chrono;
    smf::LOG_INFO("Creating connections");
    auto begin = high_resolution_clock::now();
    return connect()
      .then([this, num_of_req] {
        smf::LOG_INFO("About to send `{}` requests to server", num_of_req);
        return do_for_each(boost::counting_iterator<size_t>(0),
                           boost::counting_iterator<size_t>(num_of_req),
                           [this](size_t reqno) { return send_request(); });
      })
      .then([begin, num_of_req] {
        auto end_t = high_resolution_clock::now();
        uint64_t duration_milli =
          duration_cast<milliseconds>(end_t - begin).count();
        auto qps = double(num_of_req) / std::max(duration_milli, uint64_t(1));
        smf::LOG_INFO("Test took: {}ms", duration_milli);
        smf::LOG_INFO("Queries per millisecond: {}/ms", qps);
        return make_ready_future<uint64_t>(duration_milli);
      }); // connect
  }
  future<> connect() { return client_->connect(); }
  future<> stop() {
    sstring s =
      "rpc_client_histogram_shard_" + to_sstring(engine().cpu_id()) + ".txt";
    FILE *fp = fopen(s.c_str(), "w");
    if(fp) {
      client_->get_histogram()->print(fp);
      fclose(fp);
    }
    return make_ready_future<>();
  }

  private:
  std::unique_ptr<smf_gen::fbs::rpc::SmurfStorageClient> client_;
  flatbuffers::FlatBufferBuilder builder_;

  std::unique_ptr<smf::rpc_envelope> envelope_;
};

int main(int args, char **argv, char **env) {
  smf::LOG_INFO("About to start the client");
  distributed<rpc_client_wrapper> clients;
  app_template app;
  app.add_options()("req_num", bpo::value<size_t>()->default_value(400),
                    "number of requests")(
    "rpc_port", bpo::value<uint16_t>()->default_value(11225), "rpc port")(
    "ip_address", bpo::value<std::string>()->default_value("127.0.0.1"),
    "ip address of server");
  try {
    return app.run(args, argv, [&app, &clients]() -> future<int> {

      auto &&config = app.configuration();
      const uint16_t port = config["rpc_port"].as<uint16_t>();
      const size_t req_num = config["req_num"].as<size_t>();
      const char *ip = config["ip_address"].as<std::string>().c_str();
      smf::LOG_INFO("setting up exit hooks");
      engine().at_exit([&clients] { return clients.stop(); });
      return clients.start(ip, port)
        .then([&clients, req_num] {
          return clients.map_reduce(adder<uint64_t>(),
                                    &rpc_client_wrapper::do_req, req_num)
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
