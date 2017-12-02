// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <iostream>

#include <glog/logging.h>

#include "rpc/smf_gen/cpp_generator.h"
#include "rpc/smf_gen/smf_file.h"

DEFINE_bool(print_smf_gen_to_stderr,
            false,
            "prints to stderr the outputs of the generated header");

namespace smf_gen {
bool generate(const flatbuffers::Parser &parser, const std::string &file_name) {
  int nservices = 0;
  for (auto it = parser.services_.vec.begin(); it != parser.services_.vec.end();
       ++it) {
    if (!(*it)->generated) nservices++;
  }
  if (!nservices) return true;

  smf_file fbfile(parser, file_name);

  std::string header_code =
    get_header_prologue(&fbfile) + get_header_includes(&fbfile)
    + get_header_services(&fbfile) + get_header_epilogue(&fbfile);


  if (FLAGS_print_smf_gen_to_stderr) {
    std::cerr << "Finished generating code: \n" << header_code;
  }

  return flatbuffers::SaveFile((file_name + ".smf.fb.h").c_str(), header_code,
                               false);
}

}  // namespace smf_gen
