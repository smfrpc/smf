// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
// seastar
#include <core/app-template.hh>
#include <core/distributed.hh>
// smf
#include "filesystem/wal_requests.h"
#include "filesystem/wal_writer.h"
#include "log.h"
#include "priority_manager.h"


smf::wal_write_request gen_payload(sstring str) {
  temporary_buffer<char> buf(str.size());
  std::copy(str.begin(), str.end(), buf.get_write());
  return smf::wal_write_request(
    0, std::move(buf),
    smf::priority_manager::thread_local_instance().default_priority());
}

int main(int args, char **argv, char **env) {
  smf::DLOG_INFO("About to start the client");
  app_template      app;
  smf::writer_stats wstats;
  // TODO(make distributed wal_writer)
  try {
    return app.run(args, argv, [&app, &wstats]() mutable -> future<int> {
      smf::log.set_level(seastar::log_level::debug);
      auto writer = make_lw_shared<smf::wal_writer>(".", &wstats);
      smf::DLOG_INFO("setting up exit hooks");
      engine().at_exit([writer] { return writer->close(); });
      smf::DLOG_INFO("About to open the files");
      return writer->open().then([writer]() mutable {
        auto req = gen_payload("Hello world!");
        smf::DLOG_INFO("Writing payload: {}", req.data.size());
        return writer->append(std::move(req)).then([writer](auto epoch) {
          smf::DLOG_INFO("Wrote payload at epoch: {}", epoch);
          smf::DLOG_INFO("Invalidating epoch: {}", epoch);
          return writer->invalidate(epoch).then([epoch] {
            smf::DLOG_INFO("finished invalidating epoch: {}", epoch);
            return make_ready_future<int>(0);
          });
        });
      });
    });  // app.run
  } catch (const std::exception &e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
  }
}
