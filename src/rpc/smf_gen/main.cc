// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <vector>

#include "rpc/smf_gen/idl.h"
DEFINE_string(filename, "", "filename to parse");
DEFINE_string(include_dirs, "", "extra include directories: not supported yet");

int main(int argc, char **argv, char **env) {
  google::SetUsageMessage("Generate smf services");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);

  if (FLAGS_filename.empty()) {
    LOG(ERROR) << "No filename to parse";
    exit(1);
  }

  VLOG(1) << "Parsing file: " << FLAGS_filename
          << ", Include dirs: " << FLAGS_include_dirs;

  // generate code!
  flatbuffers::IDLOptions opts;
  flatbuffers::Parser     parser(opts);
  std::string             contents;
  LOG_IF(FATAL, !flatbuffers::LoadFile(FLAGS_filename.c_str(), true, &contents))
    << "Could not LOAD file: " << FLAGS_filename;

  VLOG(1) << FLAGS_filename << " loaded.";

  std::vector<const char *> include_directories;
  auto local_include_directory = flatbuffers::StripFileName(FLAGS_filename);
  include_directories.push_back(local_include_directory.c_str());
  include_directories.push_back(nullptr);

  LOG_IF(FATAL, !parser.Parse(contents.c_str(), &include_directories[0],
                              FLAGS_filename.c_str()))
    << "Could not PARSE file: " << parser.error_;

  VLOG(1) << "File `" << FLAGS_filename << "` parsed. Generating";

  smf_gen::generate(parser, flatbuffers::StripExtension(FLAGS_filename));
  parser.MarkGenerated();
}
