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
#include "platform/log.h"
#include "seastar_io/priority_manager.h"


static const char *kPayload = "hello";

smf::wal_write_request gen_payload(seastar::sstring str) {
  seastar::temporary_buffer<char> buf(str.size());
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
  DLOG_DEBUG("About to start the client");
  seastar::app_template                      app;
  seastar::distributed<smf::write_ahead_log> w;
  try {
    return app.run(args, argv, [&]() mutable -> seastar::future<int> {
      // SET_LOG_LEVEL(seastar::log_level::debug);
      DLOG_DEBUG("setting up exit hooks");
      seastar::engine().at_exit([&] { return w.stop(); });
      DLOG_DEBUG("about to start the wal.h");

      return w
        .start(smf::wal_type::wal_type_disk_with_memory_cache,
               smf::wal_opts("."))
        .then([&w] { return w.invoke_on_all(&smf::shardable_wal::open); })
        .then([&w] {
          std::vector<uint64_t> v;
          return w
            .map_reduce(
              seastar::adder<reducible_append>(),
              [](smf::shardable_wal &x) {
                return x.append(gen_payload(kPayload)).then([&x](uint64_t i) {
                  auto &p = smf::priority_manager::thread_local_instance()
                              .default_priority();
                  smf::wal_read_request rq{
                    static_cast<int64_t>(i),
                    static_cast<int64_t>(std::strlen(kPayload)), uint32_t(0),
                    p};
                  return x.get(std::move(rq)).then([i](auto read_result) {
                    assert(read_result);
                    auto size = read_result->fragments.front().data.size();
                    assert(size == std::strlen(kPayload));
                    DLOG_DEBUG("Got result from a read with size: {}", size);
                    reducible_append ra;
                    ra.v.insert(i);
                    return std::move(ra);
                  });
                });
              })
            .then([&w](reducible_append &&ra) {
              DLOG_DEBUG("got value: {}",
                         std::vector<uint64_t>(ra.v.begin(), ra.v.end()));
              return seastar::do_for_each(
                ra.v.begin(), ra.v.end(), [&w](uint64_t i) {
                  DLOG_DEBUG("About to invalidate epoch: {}", i);
                  return w.invoke_on_all(&smf::shardable_wal::invalidate, i)
                    .then([i]() {
                      DLOG_DEBUG("Invalidated epoch `{}' on all cores", i);
                      return seastar::make_ready_future<>();
                    });
                });
            });
        })
        .then([] { return seastar::make_ready_future<int>(0); });
    });
  } catch (const std::exception &e) {
    std::cerr << "Fatal exception: " << e.what() << std::endl;
  }
}
