// Copyright 2017 Alexander Gallego
//
#pragma once

#include "platform/log.h"
#include "platform/macros.h"
#include "rpc/filters/zstd_filter.h"
#include "rpc/rpc_envelope.h"

namespace smf {
namespace load_gen {


/// TODO(agallego) - enable zstd compressor as options of enums
/// and other enums
template <typename ClientService> struct load_channel {
  using func_t = std::function<future<>(ClientService *, smf::rpc_envelope &&)>;

  load_channel(const char *ip, uint16_t port)
    : client(new ClientService(ipv4_addr{ip, port}))
    , fbb(new flatbuffers::FlatBufferBuilder()) {
    client->enable_histogram_metrics(true);
    client->incoming_filters().push_back(smf::zstd_decompression_filter());
    client->outgoing_filters().push_back(smf::zstd_compression_filter(1000));
  }

  load_channel(load_channel<ClientService> &&o)
    : client(std::move(o.client)), fbb(std::move(o.fbb)) {}

  ~load_channel() {}

  inline histogram *get_histogram() const { return client->get_histogram(); }

  future<> connect() { return client->connect(); }

  future<> invoke(uint32_t reqs, func_t func) {
    LOG_THROW_IF(reqs == 0, "bad number of requests");
    LOG_THROW_IF(fbb->GetSize() == 0, "Empty builder, don't forget to build "
                                      "flatbuffers payload. See the "
                                      "documentation.");

    return do_for_each(boost::counting_iterator<uint32_t>(0),
                       boost::counting_iterator<uint32_t>(reqs),
                       [this, func](uint32_t i) mutable {
                         smf::rpc_envelope e(fbb->GetBufferPointer(),
                                             fbb->GetSize());
                         return func(client.get(), std::move(e));
                       });
  }

  SMF_DISALLOW_COPY_AND_ASSIGN(load_channel);
  std::unique_ptr<ClientService>                  client;
  std::unique_ptr<flatbuffers::FlatBufferBuilder> fbb;
};

}  // namespace load_gen
}  // namespace smf
