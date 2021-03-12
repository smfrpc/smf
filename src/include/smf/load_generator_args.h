// Copyright 2017 Alexander Gallego
//
#pragma once

#include <boost/program_options.hpp>
#include <memory>
#include <seastar/net/tls.hh>
#include <vector>

#include <smf/log.h>

namespace smf {
// missing timeout
// tracer probability
// exponential distribution, poisson distribution of load
//
struct load_generator_args {
  load_generator_args(const char *_ip, uint16_t _port, size_t _num_of_req,
                      size_t _concurrency, size_t _memory_per_core,
                      smf::rpc::compression_flags _compression,
                      boost::program_options::variables_map _cfg)
    : ip(_ip), port(_port), num_of_req(_num_of_req), concurrency(_concurrency),
      memory_per_core(_memory_per_core), compression(_compression),
      cfg(std::move(_cfg)) {
    static const uint64_t kMinMemory = 1 << 24;  // 16MB
    LOG_THROW_IF(memory_per_core < kMinMemory && concurrency > 1,
                 "Cfg error. memory < 16MB per connection.");
  }

  const char *ip;
  uint16_t port;
  size_t num_of_req;
  size_t concurrency;
  size_t memory_per_core;
  smf::rpc::compression_flags compression;
  seastar::shared_ptr<seastar::tls::certificate_credentials> credentials;
  const boost::program_options::variables_map cfg;
};

}  // namespace smf

namespace std {
inline std::ostream &
operator<<(std::ostream &o, const smf::load_generator_args &args) {
  o << "generator_args{ip=" << args.ip << ", port=" << args.port
    << ", num_of_req=" << args.num_of_req
    << ", concurrency=" << args.concurrency
    << ", memory_per_client=" << args.memory_per_core / args.concurrency
    << ", compression=" << smf::rpc::EnumNamecompression_flags(args.compression)
    << ", cfg_size=" << args.cfg.size() << "}";
  return o;
}
}  // namespace std
