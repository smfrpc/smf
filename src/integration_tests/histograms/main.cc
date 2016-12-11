// seastar
#include <core/distributed.hh>
#include <core/app-template.hh>
// smf
#include "log.h"
#include "histogram_seastar_utils.h"


int main(int args, char **argv, char **env) {
  smf::LOG_INFO("Starting test for histogram write");
  app_template app;
  try {
    return app.run(args, argv, [&app]() -> future<int> {
      smf::log.set_level(seastar::log_level::debug);
      smf::histogram h;
      for(auto i = 0u; i < 1000; i++) {
        h.record(i*i);
      }
      smf::LOG_INFO("Writing histogram");
      return smf::histogram_seastar_utils::write_histogram("hist.testing.txt",
                                                           std::move(h))
        .then([] { return make_ready_future<int>(0); });
    }); // app.run
  } catch(const std::exception &e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
  }
}
