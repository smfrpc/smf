// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
// seastar
#include <core/app-template.hh>
#include <core/distributed.hh>
// smf
#include "histogram/histogram_seastar_utils.h"
#include "platform/log.h"

int main(int args, char **argv, char **env) {
  LOG_DEBUG("Starting test for histogram write");
  seastar::app_template app;
  try {
    return app.run(args, argv, [&app]() -> seastar::future<int> {
      smf::histogram h;
      for (auto i = 0u; i < 1000; i++) {
        h.record(i * i);
      }
      LOG_DEBUG("Writing histogram");
      return smf::histogram_seastar_utils::write_histogram("hist.testing.hgrm",
                                                           std::move(h))
        .then([] { return seastar::make_ready_future<int>(0); });
    });  // app.run
  } catch (const std::exception &e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
  }
}
