#include <iostream>
#include <stdexcept>
#include "core/app-template.hh"
#include "rpc/rpc_server.h"

int main(int argc, char **argv) {
  distributed<smf::rpc_server> rpc_server;
  app_template app;
  try {
    app.run(argc, argv, f);
  } catch(const std::runtime_error &e) {
    std::cerr << "Couldn't start application: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
