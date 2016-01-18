#include "core/app-template.hh"
#include "core/reactor.hh"
#include <iostream>
#include <stdexcept>
int main(int argc, char **argv) {
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
