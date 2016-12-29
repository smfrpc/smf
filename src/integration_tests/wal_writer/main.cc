// seastar
#include <core/distributed.hh>
#include <core/app-template.hh>
// smf
#include "log.h"
#include "filesystem/wal_writer.h"


temporary_buffer<char> gen_payload(sstring str) {
  temporary_buffer<char> buf(str.size());
  std::copy(str.begin(), str.end(), buf.get_write());
  return std::move(buf);
}

int main(int args, char **argv, char **env) {
  smf::DLOG_INFO("About to start the client");
  app_template app;
  // TODO(make distributed wal_writer)
  // add flags
  try {
    return app.run(args, argv, [&app]() -> future<int> {
      smf::log.set_level(seastar::log_level::debug);
      auto writer = make_lw_shared<smf::wal_writer>(".");
      smf::DLOG_INFO("setting up exit hooks");
      engine().at_exit([writer] { return writer->close(); });
      return writer->open().then([writer]() mutable {
        auto buf = gen_payload("Hello world!");
        sstring tmp(buf.get(), buf.size());
        smf::DLOG_INFO("Writing payload: {}", tmp);
        return writer->append(std::move(buf))
          .then([] { return make_ready_future<int>(0); });
      });
    }); // app.run
  } catch(const std::exception &e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
  }
}
