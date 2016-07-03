#include <iostream>
#include <vector>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>

#include "rpc/smf_gen/idl.h"
DEFINE_string(filename, "", "filename to parse");
DEFINE_string(include_dirs, "", "extra include directories: not supported yet");

int main(int argc, char **argv, char **env) {
  gflags::SetUsageMessage("Generate smf services");
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);

  if(FLAGS_filename.empty()) {
    LOG(ERROR) << "No filename to parse";
    exit(1);
  }

  LOG(INFO) << "Parsing. Filename: " << FLAGS_filename
            << ", Include dirs: " << FLAGS_include_dirs;

  // generate code!
  flatbuffers::IDLOptions opts;
  flatbuffers::Parser parser(opts);
  std::string contents;
  LOG_IF(FATAL, !flatbuffers::LoadFile(FLAGS_filename.c_str(), true, &contents))
    << "Could not LOAD file: " << FLAGS_filename;

  LOG(INFO) << "File loaded.";

  std::vector<const char *> include_directories;
  auto local_include_directory = flatbuffers::StripFileName(FLAGS_filename);
  include_directories.push_back(local_include_directory.c_str());
  include_directories.push_back(nullptr);

  LOG_IF(FATAL, !parser.Parse(contents.c_str(), &include_directories[0],
                              FLAGS_filename.c_str()))
    << "Could not PARSE file: " << parser.error_;

  LOG(INFO) << "File parsed. Generating";

  smf_gen::generate(parser, flatbuffers::StripExtension(FLAGS_filename));
}
