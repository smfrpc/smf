// std
#include <iostream>
#include <chrono>
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
  rpc_client_wrapper(const char *ip, uint16_t port, size_t req_num)
    : num_of_req_(req_num) {
    smf::LOG_INFO("setting up the client");
    client_ = make_lw_shared<smf_gen::fbs::rpc::SmurfStorageClient>(
      ipv4_addr{ip, port});
  }

  future<> send_request() {
    auto req = smf_gen::fbs::rpc::CreateRequest(
      builder_, builder_.CreateString("hello from client"));
    builder_.Finish(req);
    smf::rpc_envelope env(builder_.GetBufferPointer(), builder_.GetSize());

    builder_.Clear(); // reuse it

    auto begin_send_t = std::chrono::high_resolution_clock::now();

    return client_->GetSend(std::move(env))
      .then([begin_send_t](auto reply) mutable {
        if(reply) {
          auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::high_resolution_clock::now() - begin_send_t)
              .count();
          smf::LOG_INFO(
            "Got reply from server with status: {} in {} microseconds",
            reply.ctx->status(), duration);
        } else {
          smf::LOG_INFO("Well.... the server had an OoOps!");
        }
      })
      .then([this]() mutable {
        if(this->num_of_req_-- > 0) {
          smf::LOG_INFO("Sending req: {}", num_of_req_);
          return this->send_request();
        } else {
          return make_ready_future<>();
        }
      });
  }

  future<> stop() { return make_ready_future<>(); }

  private:
  lw_shared_ptr<smf_gen::fbs::rpc::SmurfStorageClient> client_;
  flatbuffers::FlatBufferBuilder builder_;
  size_t num_of_req_;
};

int main(int args, char **argv, char **env) {
  std::cerr << "About to start the client" << std::endl;
  distributed<rpc_client_wrapper> clients;
  app_template app;
  app.add_options()("rpc_port", bpo::value<uint16_t>()->default_value(11225),
                    "rpc port")(
    "ip_address", bpo::value<std::string>()->default_value("localhost"),
    "ip address of server");
  try {
    auto &&config = app.configuration();
    uint16_t port = config["rpc_port"].as<uint16_t>();
    const char *ip = config["ip_address"].as<std::string>().c_str();
    return app.run_deprecated(args, argv, [&] {
      smf::LOG_INFO("setting up exit hooks");
      engine().at_exit([&] { return clients.stop(); });
      return clients.start(ip, port, 400)
        .then([&clients] {
          smf::LOG_INFO(
            "About to send a distributed set of requests to server");
          return clients.invoke_on_all(&rpc_client_wrapper::send_request);
        });
    });
  } catch(...) {
    std::cerr << "Fatal exception: " << std::current_exception() << std::endl;
  }
}
