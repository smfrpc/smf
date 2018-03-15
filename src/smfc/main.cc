// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <iostream>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "smfc/idl.h"

DEFINE_string(filename, "", "filename to parse");
DEFINE_string(include_dirs, "", "extra include directories: not supported yet");

std::tuple<std::vector<std::string>, std::vector<const char *>>
include_directories_split(
  const std::string &dirs, const std::string &current_filename) {
  std::vector<std::string> retval;
  boost::algorithm::split(retval, dirs, boost::is_any_of(","));
  for (auto &s : retval) { boost::algorithm::trim(s); }
  retval.push_back(flatbuffers::StripFileName(current_filename));
  // UGH THE FLATBUFFERS C-looking API sucks. :'(
  //
  std::vector<const char *> noown;
  noown.reserve(retval.size() + 1);
  for (auto &s : retval) { noown.push_back(s.c_str()); }
  // always end in null :'(
  noown.push_back(nullptr);
  return {std::move(retval), std::move(noown)};
}

int
main(int argc, char **argv, char **env) {
  google::SetUsageMessage("Generate smf services");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);

  if (FLAGS_filename.empty()) {
    LOG(ERROR) << "No filename to parse";
    std::exit(1);
  }
  if (!boost::filesystem::exists(FLAGS_filename)) {
    LOG(ERROR) << " ` " << FLAGS_filename
               << " ' - does not exists or could not be found";
    std::exit(1);
  }
  VLOG(1) << "Parsing file: " << FLAGS_filename
          << ", Include dirs: " << FLAGS_include_dirs;
  // generate code!
  flatbuffers::IDLOptions opts;
  flatbuffers::Parser parser(opts);
  std::string contents;
  if (!flatbuffers::LoadFile(FLAGS_filename.c_str(), true, &contents)) {
    LOG(ERROR) << "Could not load file: " << FLAGS_filename;
    std::exit(1);
  }
  VLOG(1) << FLAGS_filename << " loaded.";
  auto includes = include_directories_split(FLAGS_include_dirs, FLAGS_filename);
  if (!parser.Parse(
        contents.c_str(), &std::get<1>(includes)[0], FLAGS_filename.c_str())) {
    LOG(ERROR) << "Could not PARSE file: " << parser.error_;
    std::exit(1);
  }

  VLOG(1) << "File `" << FLAGS_filename << "` parsed. Generating";

  if (!smf_gen::generate(parser, flatbuffers::StripExtension(FLAGS_filename))) {
    LOG(ERROR) << "Could not generate file";
    std::exit(1);
  }
  parser.MarkGenerated();
}
