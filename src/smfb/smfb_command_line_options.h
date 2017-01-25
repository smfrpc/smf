// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <boost/program_options.hpp>
namespace smf {
void smfb_add_command_line_options(boost::program_options::variables_map *m,
                                   int                                    argc,
                                   char **                                argv);

}  // end namespace smf
