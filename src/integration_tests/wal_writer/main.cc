// seastar
#include <core/distributed.hh>
#include <core/app-template.hh>
// smf
#include "log.h"
#include "filesystem/wal_writer.h"


temporary_buffer<char> gen_payload(size_t size = 12) {
  temporary_buffer<char> buf(size);
  std::fill(buf.get_write(), buf.get_write() + buf.size(), '$');
  return std::move(buf);
}

int main(int args, char **argv, char **env) {
  smf::LOG_INFO("About to start the client");
  app_template app;
  // TODO(make distributed wal_writer)
  // add flags
  try {
    return app.run(args, argv, [&app]() -> future<int> {
      smf::log.set_level(seastar::log_level::debug);
      auto writer = make_lw_shared<smf::wal_writer>(".");
      smf::LOG_INFO("setting up exit hooks");
      engine().at_exit([writer] { return writer->close(); });
      return writer->open().then([writer]() mutable {
        smf::LOG_INFO("Writing payload");
        return writer->append(gen_payload())
          .then([] { return make_ready_future<int>(0); });
      });
    }); // app.run
  } catch(const std::exception &e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
  }
}
