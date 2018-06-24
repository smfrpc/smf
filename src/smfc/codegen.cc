// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//

#include "codegen.h"

#include <iostream>

#include <glog/logging.h>
#include <boost/filesystem.hpp>

#include "cpp_generator.h"
#include "go_generator.h"
#include "java_generator.h"

namespace smf_gen {

codegen::codegen(std::string ifname,
  std::string out_dir,
  std::vector<std::string> includes,
  std::vector<language> langs)
  : input_filename(std::move(ifname))
  , output_dir(std::move(out_dir))
  , languages(std::move(langs))
  , include_dirs_(std::move(includes)) {
  LOG_IF(FATAL, languages.empty()) << "No programming langues";
  LOG_IF(FATAL, output_dir.empty()) << "No place to put generated code";


  parser_ = std::make_unique<flatbuffers::Parser>(opts_);

  // make sure that the current file's dir is added to the include dirs
  //
  if (include_dirs_.end() ==
      std::find_if(include_dirs_.begin(), include_dirs_.end(),
        [this](auto &s) { return s == input_filename; })) {
    include_dirs_.push_back(flatbuffers::StripFileName(input_filename));
  }
}


std::size_t
codegen::service_count() const {
  if (!parsed_) {
    LOG(ERROR) << "Generator not parsed, please call ::parse() first";
    return 0;
  }
  return parser_->services_.vec.size();
}

std::experimental::optional<std::string>
codegen::parse() {
  if (parsed_) {
    if (!parser_->error_.empty()) {
      return "Parser in error state: " + parser_->error_;
    }
    return std::experimental::nullopt;
  }
  parsed_ = true;

  // UGH!!! Flatbuffers C-looking API .... is...
  //
  std::vector<const char *> cincludes;
  cincludes.reserve(include_dirs_.size() + 1);
  for (auto &s : include_dirs_) { cincludes.push_back(s.c_str()); }
  // always end in null :'(
  cincludes.push_back(nullptr);

  std::string contents;
  if (!flatbuffers::LoadFile(input_filename.c_str(), true, &contents)) {
    return "Could not load file: " + input_filename;
  }

  if (!parser_->Parse(
        contents.c_str(), cincludes.data(), input_filename.c_str())) {
    return "Could not PARSE file: " + parser_->error_;
  }

  return std::experimental::nullopt;
}


std::experimental::optional<std::string>
codegen::gen() {
  auto x = parse();
  if (x) { return x; }
  // Since language is only known at runtime time we can't use
  // template expressions
  auto lang_ptr_fn = [this](auto l) -> std::unique_ptr<generator> {
    switch (l) {
    case language::cpp: {
      return std::make_unique<cpp_generator>(
        *parser_.get(), input_filename, output_dir);
    }
    case language::go: {
      return std::make_unique<go_generator>(
        *parser_.get(), input_filename, output_dir);
    }
    case language::java: {
      return std::make_unique<java_generator>(
        *parser_.get(), input_filename, output_dir);
    }
    default:
      return nullptr;
    }
  };
  for (const auto &l : languages) {
    auto lang = lang_ptr_fn(l);
    if (lang) {
      x = lang->gen();
      if (x) { return x; }
      VLOG(1) << "Generated: " << lang->output_filename();
    } else {
      LOG(ERROR) << "Uknown language";
    }
  }
  return std::experimental::nullopt;
}

}  // namespace smf_gen
