// Copyright 2017 Alexander Gallego
//
#pragma once

#include <core/shared_ptr.hh>

#include "platform/log.h"
#include "platform/macros.h"
#include "rpc/filters/zstd_filter.h"
#include "rpc/rpc_envelope.h"

namespace smf {
namespace load_gen {


template <typename ClientService> struct load_channel {
  using func_t =
    std::function<seastar::future<>(ClientService *, smf::rpc_envelope &&)>;
  using generator_t = std::function<smf::rpc_envelope(
    const boost::program_options::variables_map &)>;

  load_channel(const char *ip, uint16_t port)
    : client(new ClientService(seastar::ipv4_addr{ip, port}))
    , fbb(new flatbuffers::FlatBufferBuilder()) {
    client->enable_histogram_metrics(true);
    /// TODO(agallego) - enable zstd compressor as options of enums
    client->incoming_filters().push_back(smf::zstd_decompression_filter());
    client->outgoing_filters().push_back(smf::zstd_compression_filter(1000));
  }

  load_channel(load_channel<ClientService> &&o)
    : client(std::move(o.client)), fbb(std::move(o.fbb)) {}

  ~load_channel() {}

  inline histogram *get_histogram() const { return client->get_histogram(); }

  seastar::future<> connect() { return client->connect(); }

  seastar::future<> invoke(uint32_t                                     reqs,
                           const boost::program_options::variables_map &opts,
                           generator_t                                  gen,
                           func_t                                       func) {
    LOG_THROW_IF(reqs == 0, "bad number of requests");

    // explicitly make copies of opts, gen, and func
    // happens once per reqs call
    //
    return seastar::do_for_each(boost::counting_iterator<uint32_t>(0),
                                boost::counting_iterator<uint32_t>(reqs),
                                [this, opts, gen, func](uint32_t i) mutable {
                                  return func(client.get(), gen(opts));
                                });
  }

  SMF_DISALLOW_COPY_AND_ASSIGN(load_channel);
  std::unique_ptr<ClientService>                  client;
  std::unique_ptr<flatbuffers::FlatBufferBuilder> fbb;
};

}  // namespace load_gen
}  // namespace smf
