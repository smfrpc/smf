// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <boost/program_options.hpp>
namespace smf {
struct smfb_command_line_options {
  static void add(boost::program_options::options_description_easy_init);
  static void validate(const boost::program_options::variables_map &vm);
};


}  // end namespace smf
