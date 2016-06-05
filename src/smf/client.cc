// std
#include <iostream>
// seastar
#include <core/app-template.hh>
// smf

int main(int args, char **argv, char **env) {
  std::cerr << "About to start the client" << std::endl;
  app_template app;
  return app.run_deprecated(args, argv, [&] {

  });
}
