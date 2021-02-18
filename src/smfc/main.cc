// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <iostream>
#include <memory>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "codegen.h"

DEFINE_string(filename, "", "filename to parse");
DEFINE_string(include_dirs, "", "extra include directories");
DEFINE_string(language, "cpp",
              "coma separated list of language to generate: go, cpp");
DEFINE_string(output_path, ".", "output path of the generated files");

std::vector<std::string>
split_coma(const std::string &dirs) {
  std::vector<std::string> retval;
  boost::algorithm::split(retval, dirs, boost::is_any_of(","));
  if (retval.empty() && !dirs.empty()) { retval.push_back(dirs); }
  for (auto &s : retval) {
    boost::algorithm::trim(s);
  }
  return retval;
}

std::vector<smf_gen::language>
split_langs(const std::string &lang) {
  // Note: be sure to add the generator in codegen.cc
  std::vector<smf_gen::language> retval;
  auto str_langs = split_coma(lang);
  for (auto &l : str_langs) {
    if (l == "cpp") {
      retval.push_back(smf_gen::language::cpp);
    } else if (l == "go") {
      retval.push_back(smf_gen::language::go);
    } else if (l == "python") {
      retval.push_back(smf_gen::language::python);
    } else {
      LOG(ERROR) << "Skipping unknown language: " << l;
    }
  }
  return retval;
}

int
main(int argc, char **argv, char **env) {
  google::SetUsageMessage("Generate smf services");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);

  // validate flags
  if (FLAGS_filename.empty()) {
    LOG(ERROR) << "No filename to parse";
    std::exit(1);
  }
  if (!boost::filesystem::exists(FLAGS_filename)) {
    LOG(ERROR) << " ` " << FLAGS_filename
               << " ' - does not exists or could not be found";
    std::exit(1);
  }
  if (FLAGS_output_path.empty()) {
    LOG(ERROR) << " ` " << FLAGS_output_path << " ' - empty output path";
    std::exit(1);
  }
  FLAGS_output_path =
    boost::filesystem::canonical(FLAGS_output_path.c_str()).string();
  if (!flatbuffers::DirExists(FLAGS_output_path.c_str())) {
    LOG(ERROR) << "--output_path specified, but directory: "
               << FLAGS_output_path << " does not exist;";
    std::exit(1);
  }

  auto codegenerator = std::make_unique<smf_gen::codegen>(
    FLAGS_filename, FLAGS_output_path, split_coma(FLAGS_include_dirs),
    split_langs(FLAGS_language));
  // generate code!
  auto status = codegenerator->parse();
  if (status) {
    LOG(ERROR) << "Failed to parse file: " << status.value();
    std::exit(1);
  }
  if (codegenerator->service_count() == 0) {
    LOG(INFO) << "No services need to be generated";
    // if we return 0, the cmake module cannot detect if we generate a file or
    // not and always calls smfc return 0;
  }
  status = codegenerator->gen();
  if (status) {
    LOG(ERROR) << "Errors generating code: " << *status;
    std::exit(1);
  }
  VLOG(1) << "Success";
  return 0;
}
