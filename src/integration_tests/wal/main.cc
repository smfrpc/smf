// Copyright (c) 2017 Alexander Gallego. All rights reserved.
//
#include <set>
// seastar
#include <core/app-template.hh>
#include <core/distributed.hh>
// smf
#include "filesystem/wal.h"
#include "filesystem/wal_opts.h"
#include "filesystem/wal_requests.h"
#include "priority_manager.h"
// log
#include "log.h"


static const char *kPayload = "hello";

smf::wal_write_request gen_payload(sstring str) {
  temporary_buffer<char> buf(str.size());
  std::copy(str.begin(), str.end(), buf.get_write());
  return smf::wal_write_request(
    0, std::move(buf),
    smf::priority_manager::thread_local_instance().default_priority());
}

struct reducible_append {
  reducible_append() {}
  reducible_append &operator=(const reducible_append &o) {
    v.clear();
    v.insert(o.v.begin(), o.v.end());
    return *this;
  }
  reducible_append(reducible_append &&o) noexcept : v(std::move(o.v)) {}
  reducible_append &operator+=(const reducible_append &o) {
    v.insert(o.v.begin(), o.v.end());
    return *this;
  }
  std::set<uint64_t> v;
};

int main(int args, char **argv, char **env) {
  smf::DLOG_INFO("About to start the client");
  app_template                    app;
  distributed<smf::shardable_wal> w;
  try {
    return app.run(args, argv, [&]() mutable -> future<int> {
      smf::log.set_level(seastar::log_level::debug);
      smf::DLOG_INFO("setting up exit hooks");
      engine().at_exit([&] { return w.stop(); });
      smf::DLOG_INFO("about to start the wal.h");

      return w
        .start(smf::wal_type::wal_type_disk_with_memory_cache,
               smf::wal_opts("."))
        .then([&w] { return w.invoke_on_all(&smf::shardable_wal::open); })
        .then([&w] {
          std::vector<uint64_t> v;
          return w
            .map_reduce(
              adder<reducible_append>(),
              [](smf::shardable_wal &x) {
                return x.append(gen_payload(kPayload)).then([&x](uint64_t i) {
                  auto &p = smf::priority_manager::thread_local_instance()
                              .default_priority();
                  smf::wal_read_request rq{i, (uint64_t)std::strlen(kPayload),
                                           uint32_t(0), p};
                  return x.get(std::move(rq)).then([i](auto read_result) {
                    assert(read_result);
                    assert(read_result->size() == std::strlen(kPayload));
                    smf::DLOG_INFO("Got result from a read with size: {}",
                                   read_result->size());
                    reducible_append ra;
                    ra.v.insert(i);
                    return std::move(ra);
                  });
                });
              })
            .then([&w](reducible_append &&ra) {
              smf::DLOG_INFO("got value: {}",
                             std::vector<uint64_t>(ra.v.begin(), ra.v.end()));
              return do_for_each(ra.v.begin(), ra.v.end(), [&w](uint64_t i) {
                smf::DLOG_INFO("About to invalidate epoch: {}", i);
                return w.invoke_on_all(&smf::shardable_wal::invalidate, i)
                  .then([i]() {
                    smf::DLOG_INFO("Invalidated epoch `{}' on all cores", i);
                    return make_ready_future<>();
                  });
              });
            });
        })
        .then([] { return make_ready_future<int>(0); });
    });
  } catch (const std::exception &e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
  }
}
