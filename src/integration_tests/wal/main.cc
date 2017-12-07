// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#include <set>
// seastar
#include <core/app-template.hh>
#include <core/distributed.hh>
#include <core/sstring.hh>

#include <flatbuffers/minireflect.h>

// smf
#include "filesystem/wal_opts.h"
#include "filesystem/wal_pretty_print_utils.h"
#include "filesystem/wal_requests.h"
#include "filesystem/write_ahead_log.h"
#include "flatbuffers/fbs_typed_buf.h"
#include "platform/log.h"

// tests
#include "integration_tests/wal/wal_test_put.h"

using namespace smf;  // NOLINT

smf::wal::tx_get_requestT
get_read_request(uint32_t partition = 0) {
  smf::wal::tx_get_requestT r;
  r.topic     = "dummy_topic_test";
  r.partition = partition;
  r.offset    = 0;
  r.max_bytes = 1024;
  return r;
}

void
add_opts(boost::program_options::options_description_easy_init o) {
  namespace po = boost::program_options;
  o("write-ahead-log-dir", po::value<std::string>()->default_value("."),
    "log directory");
}


seastar::future<>
do_one_request(uint32_t core, smf::write_ahead_log &w) {
  return seastar::do_with(smf::wal_test_put(100, 10), [
    &w, running_core = core, assigned_core = 1000 + core
  ](wal_test_put & put) {
    return w.append(put.get_request(assigned_core, running_core))
      .then([&](auto write_reply) mutable {
        auto readq = smf::fbs_typed_buf<smf::wal::tx_get_request>(
          smf::native_table_as_buffer<smf::wal::tx_get_request>(
            get_read_request(put.get_partition())));

        // perform the read next!
        return seastar::do_with(
          std::move(readq),
          [&w](smf::fbs_typed_buf<smf::wal::tx_get_request> &tbuf) {
            smf::wal_read_request r(
              tbuf.get(), smf::priority_manager::get().default_priority());
            return w.get(r);
          });
      })
      .then([](seastar::lw_shared_ptr<smf::wal_read_reply> r) {
        LOG_INFO("Reply for reading: {}", *r.get());
        for (auto &g : r->reply()->gets) {
          auto ptr =
            flatbuffers::GetRoot<smf::wal::tx_put_fragment>(g->fragment.data());
          LOG_THROW_IF(ptr == nullptr, "Could not flatbuffers::GetRoot");
          LOG_THROW_IF(ptr->key()->Data() == nullptr,
                       "Ooops, Likely incorrect flatbuffers format "
                       "on the wire!");
        }
        return seastar::make_ready_future<>();
      });
  });
}

int
main(int args, char **argv, char **env) {
  DLOG_DEBUG("About to start the client");
  seastar::app_template                      app;
  seastar::distributed<smf::write_ahead_log> w;

  try {
    add_opts(app.add_options());

    return app.run(args, argv, [&] {
      SET_LOG_LEVEL(seastar::log_level::trace);
      DLOG_DEBUG("setting up exit hooks");
      seastar::engine().at_exit([&] { return w.stop(); });
      DLOG_DEBUG("about to start the wal.h");
      auto &config = app.configuration();
      auto  dir    = config["write-ahead-log-dir"].as<std::string>();

      return w.start(smf::wal_opts(dir))
        .then([&] { return w.invoke_on_all(&smf::write_ahead_log::open); })
        .then([&] {
          return seastar::do_for_each(
            boost::counting_iterator<uint32_t>(0),
            boost::counting_iterator<uint32_t>(seastar::smp::count),
            [&](auto tid) mutable {
              auto core = tid % seastar::smp::count;
              return seastar::smp::submit_to(
                core, [&, core]() { return do_one_request(core, w.local()); });
            });
        })
        .then([] { return seastar::make_ready_future<int>(0); });
    });
  } catch (const std::exception &e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
  }
}
