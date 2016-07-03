// std
#include <iostream>
// seastar
#include <core/app-template.hh>
// smf
#include "log.h"
#include "rpc/rpc_client.h"

int main(int args, char **argv, char **env) {
  std::cerr << "About to start the client" << std::endl;
  app_template app;
  try {
    return app.run_deprecated(args, argv, [&] {
      smf::LOG_INFO("setting up the client");
      smf::rpc_client client(ipv4_addr{"127.0.0.1", 11225});
      smf::LOG_INFO("Client all set up... making request");
      smf::rpc_envelope req("hello");
      req.set_request_id(1,2);
      smf::LOG_INFO("About to send rpc request to server");
      client.send(std::move(req));
    });
  } catch(...) {
    std::cerr << "Fatal exception: " << std::current_exception() << std::endl;
  }
}
