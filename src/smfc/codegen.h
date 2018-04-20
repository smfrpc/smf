// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include <experimental/optional>
#include <memory>

#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/idl.h>

namespace smf_gen {

enum class language { none, cpp, go, all };

class codegen {
 public:
  using status = std::experimental::optional<std::string>;

  codegen(std::string ifname,
    std::string output_dir,
    std::vector<std::string> include_dirs,
    std::vector<language> langs);
  ~codegen() = default;

  status gen();

  const std::string input_filename;
  const std::string output_dir;
  const std::vector<language> languages;

 private:
  status parse();

 private:
  std::vector<std::string> include_dirs_;
  flatbuffers::IDLOptions opts_;
  std::unique_ptr<flatbuffers::Parser> parser_;
  bool parsed_ = false;
};
}  // namespace smf_gen
