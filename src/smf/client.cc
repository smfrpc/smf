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
    [](const sstring &log) { printf("%s\n", log.c_str()); });

  return app.run_deprecated(argc, argv, [&] {
    auto client = std::make_unique<rpc::protocol<person_serializer>::client>(
      person_protocol, ipv4_addr{8000});
    printf("starting client!\n");
    auto get_person_rpc = person_protocol.make_client<person()>(1 /*slot*/);
    auto f =
      get_person_rpc(*client)
        .then([](person p) { printf("Got person name: %s", p.name.c_str()); })
        .then_wrapped([](future<> f) {
          try {
            f.get();
            printf("test8 should not get here!\n");
          } catch(rpc::timeout_error) {
            printf("test8 timeout!\n");
          }
        });
    f.finally([&] {
      sleep(1s).then([&] { client->stop().then([] { engine().exit(0); }); });
    });
  });
}
