// Copyright 2017 Alexander Gallego
//
#pragma once

#include <boost/program_options.hpp>
#include <memory>
#include <vector>

namespace smf {
namespace load_gen {
// missing timeout
// tracer probability
// exponential distribution, poisson distribution of load
//
struct generator_args {
  generator_args(const char *                          _ip,
                 uint16_t                              _port,
                 size_t                                _num_of_req,
                 size_t                                _concurrency,
                 boost::program_options::variables_map _cfg)
    : ip(_ip)
    , port(_port)
    , num_of_req(_num_of_req)
    , concurrency(_concurrency)
    , cfg(std::move(_cfg)) {}

  const char *                                ip;
  uint16_t                                    port;
  size_t                                      num_of_req;
  size_t                                      concurrency;
  const boost::program_options::variables_map cfg;
};

std::ostream &operator<<(std::ostream &o, const generator_args &args) {
  o << "generator_args{ip=" << args.ip << ",port=" << args.port
    << ",num_of_req=" << args.num_of_req << ",concurrency=" << args.concurrency
    << ",cfg_size=" << args.cfg.size() << "}";
  return o;
}

}  // namespace load_gen
}  // namespace smf
