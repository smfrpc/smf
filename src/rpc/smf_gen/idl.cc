// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <iostream>

#include <boost/filesystem.hpp>
#include <glog/logging.h>

#include "rpc/smf_gen/cpp_generator.h"
#include "rpc/smf_gen/smf_file.h"

DEFINE_bool(print_smf_gen_to_stderr,
            false,
            "prints to stderr the outputs of the generated header");
DEFINE_string(output_path, "", "output path of the generated files");


namespace smf_gen {

bool generate(const flatbuffers::Parser &parser, std::string file_name) {
  smf_file fbfile(parser, file_name);

  std::string header_code =
    get_header_prologue(&fbfile) + get_header_includes(&fbfile)
    + get_header_services(&fbfile) + get_header_epilogue(&fbfile);


  if (FLAGS_print_smf_gen_to_stderr) { std::cerr << header_code << std::endl; }
  if (!FLAGS_output_path.empty()) {
    FLAGS_output_path =
      boost::filesystem::canonical(FLAGS_output_path.c_str()).string();
    if (!flatbuffers::DirExists(FLAGS_output_path.c_str())) {
      LOG(ERROR) << "--output_path specified, but directory: "
                 << FLAGS_output_path << " does not exist;";
      return false;
    }
    boost::filesystem::path p(file_name);
    file_name = p.filename().string();
  }
  const std::string fname = FLAGS_output_path.empty() ?
                              file_name + ".smf.fb.h" :
                              FLAGS_output_path + "/" + file_name + ".smf.fb.h";
  LOG(INFO) << fname;

  return flatbuffers::SaveFile(fname.c_str(), header_code, false);
}

}  // namespace smf_gen
