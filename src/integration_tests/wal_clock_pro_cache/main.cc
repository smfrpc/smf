// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
// seastar
#include <core/app-template.hh>
#include <core/distributed.hh>
// smf
#include "filesystem/wal_pretty_print_utils.h"
#include "filesystem/wal_reader.h"
#include "filesystem/wal_requests.h"
#include "filesystem/wal_writer.h"
#include "platform/log.h"
#include "seastar_io/priority_manager.h"

/*
  // TODO these tests don't make sense, need to write new ones

seastar::future<> write(uint32_t max) {
  auto wstats = seastar::make_lw_shared<smf::writer_stats>();
  auto writer = seastar::make_lw_shared<smf::wal_writer>(".", wstats.get());

  return writer->open()
    .then([wstats, writer, max] {
      return seastar::do_for_each(
        boost::counting_iterator<uint32_t>(0),
        boost::counting_iterator<uint32_t>(max), [wstats, writer](auto i) {
          return writer->append(gen_payload()).then([writer](auto epoch) {
            return seastar::make_ready_future<>();
          });
        });
    })
    .then([writer, wstats] {
      return writer->close().finally([writer, wstats] {});
    });
}

smf::wal_read_request gen_read_request() {
  static const ::seastar::io_priority_class &pc =
    smf::priority_manager::thread_local_instance().default_priority();
  return smf::wal_read_request(uint64_t(0), uint64_t(1000000), uint32_t(0), pc);
}


seastar::future<> read(uint32_t expected_heder_count) {
  auto rstats = seastar::make_lw_shared<smf::reader_stats>();
  auto reader = seastar::make_lw_shared<smf::wal_reader>(".", rstats.get());
  return reader->open()
    .then([reader, rstats, expected_heder_count]() mutable {
      auto r = gen_read_request();
      DLOG_INFO("Read request: {}", r);
      return reader->get(r).then([reader, expected_heder_count](auto maybe) {
        DLOG_THROW_IF(!maybe, "eeeee... supposed to have some");
        DLOG_INFO("fragments: {}", maybe->fragments.size());
        DLOG_THROW_IF(expected_heder_count != maybe->fragments.size(),
                      "fragment size does not match expected fragment count");
        DLOG_INFO("Size of read request: {}", maybe->size());
        return seastar::make_ready_future<>();
      });
    })
    .then([reader, rstats] {
      return reader->close().finally([reader, rstats] {});
    });
}

static const uint32_t kNumberOfAppends = 1000;
int main(int args, char **argv, char **env) {
  seastar::app_template app;
  try {
    return app.run(args, argv, [&]() mutable -> seastar::future<int> {
      return write(kNumberOfAppends)
        .then([] { return read(kNumberOfAppends); })
        .then([] { return seastar::make_ready_future<int>(0); });
    });
  } catch (const std::exception &e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
  }
}
*/

int main(int argc, char **argv) { return 0; }
