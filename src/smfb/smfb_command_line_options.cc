// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include "smfb/smfb_command_line_options.h"

#include <core/sstring.hh>

namespace smf {
boost::program_options::options_description smfb_options() {
  namespace po = boost::program_options;
  po::options_description opts("smfb broker options");
  auto                    o = opts.add_options();
  o("rpc_port", po::value<uint16_t>()->default_value(11201), "rpc port");
  o("write_ahead_log_dir", po::value<sstring>(), "log directory");

  o("rpc_stats_period_mins", po::value<int>()->default_value(5),
    "period to print stats in minutes");

  o("wal_stats_period_mins", po::value<int>()->default_value(15),
    "period to print stats in minutes");

  o("log_level", po::value<sstring>()->default_value("info"),
    "info | debug | trace");

  return opts;
}

void validate_options(const boost::program_options::variables_map &vm) {
  // fix rand_home var, etc.
}


void smfb_add_command_line_options(boost::program_options::variables_map *vm,
                                   int                                    argc,
                                   char **argv) {
  namespace po = boost::program_options;
  po::store(po::command_line_parser(argc, argv).options(smfb_options()).run(),
            *vm);
  po::notify(*vm);
  validate_options(*vm);
}


}  // end namespace smf
