#include <iostream>

#include "core/app-template.hh"
#include "core/reactor.hh"
#include "core/sleep.hh"
#include "rpc/rpc.hh"

#include "person.h"


using namespace std::chrono_literals;

int main(int argc, char **argv) {
  app_template app;

  rpc::protocol<person_serializer> person_protocol(person_serializer{});
  // if you don't set the logger, it won't print anything
  person_protocol.set_logger(
    [](const sstring &log) { printf("%.*s\n", log.size(), log.c_str()); });
  std::unique_ptr<rpc::protocol<person_serializer>::server> server;
  app.run(argc, argv, [] {
    std::cout << "Starting service\n";
    pserson_protocol.register_handle(1, []() {
      person p;
      p.name = "alex gallego";
    });

    rpc::resource_limits limits;
    limits.bloat_factor = 1;
    limits.basic_request_size = 0;
    limits.max_memory = 10'000'000;
    server = std::make_unique<rpc::protocol<serializer>::server>(
      myrpc, ipv4_addr{8000}, limits);

    return make_ready_future<>();
  });
}
