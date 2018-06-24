// Copyright 2018 SMF Authors
//

#pragma once
#include <experimental/optional>
#include <sstream>
#include <string>

#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>

#include "generator.h"

namespace smf_gen {

class java_generator : public generator {
 public:
  java_generator(const flatbuffers::Parser &p,
    const std::string &ifname,
    const std::string &output_dir)
    : generator(p, ifname, output_dir) {}
  virtual ~java_generator() = default;

  virtual std::string
  output_filename() final {
    return output_dir + "/" + input_filename_without_ext() + ".smf.fb.java";
  }

  virtual std::experimental::optional<std::string>
  gen() final {
    generate_header_prologue();
    generate_header_services();
    generate_header_epilogue();
    return save_conents_to_file();
  }

 private:
  void generate_header_prologue();
  void generate_header_epilogue();
  void generate_header_services();
};

}  // namespace smf_gen
