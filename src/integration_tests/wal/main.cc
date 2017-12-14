// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#include <set>
// seastar
#include <core/app-template.hh>
#include <core/distributed.hh>
// smf
#include "filesystem/wal_opts.h"
#include "filesystem/wal_requests.h"
#include "filesystem/write_ahead_log.h"
#include "platform/log.h"

// tests
#include "integration_tests/wal/wal_test_put.h"

int main(int args, char **argv, char **env) {
  DLOG_DEBUG("About to start the client");
  seastar::app_template                      app;
  seastar::distributed<smf::write_ahead_log> w;
  smf::wal_test_put                          put_req;

  try {
    return app.run(args, argv, [&] {
      SET_LOG_LEVEL(seastar::log_level::trace);
      DLOG_DEBUG("setting up exit hooks");
      seastar::engine().at_exit([&] { return w.stop(); });
      DLOG_DEBUG("about to start the wal.h");

      return w.start(smf::wal_opts("."))
        .then([&w] { return w.invoke_on_all(&smf::write_ahead_log::open); })
        .then([&w, &put_req] {
          return w.invoke_on_all([&put_req](smf::write_ahead_log &w) {
            return w.append(put_req.get_request())
              .then([](smf::wal_write_reply r) {
                return seastar::make_ready_future<>();
              });
          });
        })
        .then([] { return seastar::make_ready_future<int>(0); });
    });
  } catch (const std::exception &e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
  }
}
