#include <iostream>
#include <iostream>
#include <stdexcept>


#include "folly/SocketAddress.h"
#include "seastar/core/app-template.hh"
#include "seastar/core/reactor.hh"

int main(int argc, char **argv) {
  folly::SocketAddress a;
  a.setFromHostPort("localhost", 2344);
  std::cout << "Test folly: Address is: " << a.describe() << std::endl;
  app_template app;
  try {
    app.run(argc, argv, [] {
      std::cout << "hello world\n";
      return make_ready_future<>();
    });
  } catch(const std::runtime_error &e) {
    std::cerr << "Couldn't start application: " << e.what() << "\n";
    return 1;
  } catch(const std::exception &e) {
    std::cerr << "wtf: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
