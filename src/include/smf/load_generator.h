// Copyright 2017 Alexander Gallego
//
#pragma once

#include <algorithm>
#include <memory>

#include <seastar/core/shared_ptr.hh>

#include "smf/load_generator_args.h"
#include "smf/load_generator_duration.h"
#include "smf/macros.h"
#include "smf/random.h"
#include "smf/rpc_envelope.h"

namespace smf {

/// \brief enable load generator for any smf::rpc_client*
/// You can only have one client per active stream
/// the problem comes when you try to read, 2 function calls to read, even
/// read_exactly might interpolate
/// which yields incorrect results for test. That use case has to be manually
/// optimized and don't expect it to be the main use case
/// In effect, you need a socket per concurrency item in the command line
/// flags
///
template <typename ClientService>
class __attribute__((visibility("default"))) load_generator {
 public:
  using channel_t = load_channel<ClientService>;
  using channel_t_ptr = std::unique_ptr<channel_t>;

  using method_cb_t =
    std::function<seastar::future<>(ClientService *, smf::rpc_envelope &&)>;
  using generator_cb_t = std::function<smf::rpc_envelope(
    const boost::program_options::variables_map &)>;

  explicit load_generator(load_generator_args _args) : args(_args) {
    random rand;
    channels_.reserve(args.concurrency);
    for (uint32_t i = 0u; i < args.concurrency; ++i) {
      channels_.push_back(
        std::make_unique<channel_t>(rand.next(), args.ip, args.port,
                                    args.memory_per_core / args.concurrency,
                                    args.compression, args.credentials));
    }
  }
  ~load_generator() {}

  SMF_DISALLOW_COPY_AND_ASSIGN(load_generator);

  const load_generator_args args;

  std::unique_ptr<smf::histogram>
  copy_histogram() const {
    auto h = smf::histogram::make_unique();
    for (auto &c : channels_) {
      auto p = c->get_histogram();
      *h += *p;
    }
    return std::move(h);
  }

  seastar::future<>
  stop() {
    return seastar::do_for_each(channels_.begin(), channels_.end(),
                                [](auto &c) { return c->stop(); });
  }
  seastar::future<>
  connect() {
    LOG_INFO("Making {} connections on this core.", channels_.size());
    return seastar::do_for_each(channels_.begin(), channels_.end(),
                                [](auto &c) { return c->connect(); });
  }
  seastar::future<load_generator_duration>
  benchmark(generator_cb_t gen, method_cb_t method_cb) {
    namespace co = std::chrono;
    const uint32_t reqs_per_channel =
      std::max<uint32_t>(1, std::ceil(args.num_of_req / args.concurrency));
    auto duration =
      seastar::make_lw_shared<load_generator_duration>(reqs_per_channel);

    return seastar::do_with(
             seastar::semaphore(args.concurrency),
             [this, duration, method_cb, gen,
              reqs_per_channel](auto &limit) mutable {
               duration->begin();
               return seastar::do_for_each(
                        channels_.begin(), channels_.end(),
                        [this, &limit, gen, method_cb, reqs_per_channel,
                         duration](auto &c) mutable {
                          return limit.wait(1).then([this, gen, &c, &limit,
                                                     reqs_per_channel, duration,
                                                     method_cb]() mutable {
                            // notice that this does not return, hence
                            // executing concurrently
                            (void)c
                              ->invoke(reqs_per_channel, args.cfg, duration,
                                       gen, method_cb)
                              .finally([&limit] { limit.signal(1); });
                          });
                        })
                 .then([this, &limit, duration] {
                   // now let's wait for ALL to finish
                   return limit.wait(args.concurrency).finally([duration] {
                     duration->end();
                   });
                 });
             })
      .then([duration] {
        return seastar::make_ready_future<load_generator_duration>(
          std::move(*duration));
      });
  }

 private:
  std::vector<channel_t_ptr> channels_{};
};

}  // namespace smf
