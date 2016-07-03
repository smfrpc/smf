#pragma once
#include "rpc/rpc_service.h"
#include "rpc/rpc_client.h"
#include "rpc/rpc_recv_context.h"
#include "log.h"

using namespace std::experimental;

// this is the type of class that gets generated
class hello_sample_service_generated : public smf::rpc_service {
  virtual const char *service_name() { return "hello sample service"; }
  virtual uint32_t service_id() { return 1234; }
  virtual std::vector<smf::rpc_service_method_handle> methods() {
    using h = smf::rpc_service_method_handle;
    std::vector<h> handles;
    handles.emplace_back(
      "hello_rpc_method", 1,
      [this](smf::rpc_recv_context &&c) -> future<smf::rpc_envelope> {
        return hello_rpc_method(std::move(c));
      });
    return handles;
  }
  virtual future<smf::rpc_envelope>
  hello_rpc_method(smf::rpc_recv_context &&recv) = 0;
};

// this is what the user implements
class hello_sample_service final : public hello_sample_service_generated {
  virtual future<smf::rpc_envelope>
  hello_rpc_method(smf::rpc_recv_context &&recv) final {
    smf::LOG_INFO("got rpc on the server");
    return make_ready_future<smf::rpc_envelope>(
      smf::rpc_envelope("hello from server"));
  }
};

// this whole class is code generated
class hello_sample_service_client final : public smf::rpc_client {
  public:
  hello_sample_service_client(ipv4_addr a) : rpc_client(a) {}

  // code generated
  future<optional<smf::rpc_recv_context>>
  send_hello_rpc_method(smf::rpc_envelope &&req) {
    req.set_request_id(1234, 1);
    return send(std::move(req), false);
  }
};
