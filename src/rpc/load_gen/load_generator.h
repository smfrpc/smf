// Copyright 2017 Alexander Gallego
//
#pragma once

#include <algorithm>
#include <memory>

#include <core/shared_ptr.hh>

#include "rpc/rpc_envelope.h"

#include "rpc/load_gen/generator_args.h"
#include "rpc/load_gen/generator_duration.h"


namespace smf {
namespace load_gen {

/// \brief enable load generator for any smf::rpc_client*
/// You can only have one client per active stream
/// the problem comes when you try to read, 2 function calls to read, even
/// read_exactly might interpolate
/// which yields incorrect results for test. That use case has to be manually
/// optimized and don't expect it to be the main use case
/// In effect, you need a socket per concurrency item in the command line
/// flags
///
template <typename ClientService> class load_generator {
 public:
  using channel_t     = load_channel<ClientService>;
  using channel_t_ptr = std::unique_ptr<channel_t>;

  using method_cb_t =
    std::function<future<>(ClientService *, smf::rpc_envelope &&)>;
  using generator_cb_t = std::function<smf::rpc_envelope(
    const boost::program_options::variables_map &)>;


  explicit load_generator(generator_args _args) : args(_args) {
    channels_.reserve(args.concurrency);
    for (uint32_t i = 0u; i < args.concurrency; ++i) {
      channels_.push_back(std::make_unique<channel_t>(args.ip, args.port));
    }
  }
  ~load_generator() {}

  const generator_args args;

  smf::histogram copy_histogram() const {
    smf::histogram h;
    for (auto &c : channels_) {
      h += *c->get_histogram();
    }
    return std::move(h);
  }

  future<> stop() { return make_ready_future<>(); }
  future<> connect() {
    return do_for_each(channels_.begin(), channels_.end(),
                       [](auto &c) { return c->connect(); });
  }
  future<generator_duration> benchmark(generator_cb_t gen,
                                       method_cb_t    method_cb) {
    namespace co = std::chrono;
    const uint32_t reqs_per_channel =
      std::max<uint32_t>(1, std::ceil(args.num_of_req / args.concurrency));
    auto duration = make_lw_shared<generator_duration>(reqs_per_channel);

    return do_with(
             semaphore(args.concurrency),
             [this, duration, method_cb, gen,
              reqs_per_channel](auto &limit) mutable {
               duration->begin();
               return do_for_each(
                        channels_.begin(), channels_.end(),
                        [this, &limit, gen, method_cb,
                         reqs_per_channel](auto &c) mutable {
                          return limit.wait(1).then([this, gen, &c, &limit,
                                                     reqs_per_channel,
                                                     method_cb]() mutable {
                            // notice that this does not return, hence
                            // executing concurrently
                            c->invoke(reqs_per_channel, args.cfg, gen,
                                      method_cb)
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
        return make_ready_future<generator_duration>(std::move(*duration));
      });
  }

 private:
  std::vector<channel_t_ptr> channels_{};
};


}  // namespace load_gen
}  // namespace smf
