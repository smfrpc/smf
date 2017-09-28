// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//

/*

// seastar
#include <core/app-template.hh>
#include <core/distributed.hh>
// smf
#include "filesystem/wal_requests.h"
#include "filesystem/wal_writer.h"
#include "platform/log.h"
#include "seastar_io/priority_manager.h"


smf::wal_write_request gen_payload(seastar::sstring str) {
  seastar::temporary_buffer<char> buf(str.size());
  std::copy(str.begin(), str.end(), buf.get_write());
  return smf::wal_write_request(
    0, std::move(buf),
    smf::priority_manager::thread_local_instance().default_priority());
}

int main(int args, char **argv, char **env) {
  DLOG_INFO("About to start the client");
  seastar::app_template app;
  smf::writer_stats     wstats;
  // TODO(make distributed wal_writer)
  try {
    return app.run(
      args, argv, [&app, &wstats]() mutable -> seastar::future<int> {
        SET_LOG_LEVEL(seastar::log_level::debug);
        auto writer = seastar::make_lw_shared<smf::wal_writer>(".", &wstats);
        DLOG_INFO("setting up exit hooks");
        seastar::engine().at_exit([writer] { return writer->close(); });
        DLOG_INFO("About to open the files");
        return writer->open().then([writer]() mutable {
          auto req = gen_payload("Hello world!");
          DLOG_INFO("Writing payload: {}", req.data.size());
          return writer->append(std::move(req)).then([writer](auto epoch) {
            DLOG_INFO("Wrote payload at epoch: {}", epoch);
              return seastar::make_ready_future<int>(0);
          });
        });
      });  // app.run
  } catch (const std::exception &e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
  }
}
*/
int main(int argc, char **argv) { return 0; }
