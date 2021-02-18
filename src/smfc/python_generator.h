// Copyright 2019 SMF Authors
//
#pragma once
#include <sstream>
#include <string>

#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>

#include "generator.h"

namespace smf_gen {

class python_generator final : public generator {
 public:
  python_generator(const flatbuffers::Parser &p, const std::string &ifname,
                   const std::string &output_dir)
    : generator(p, ifname, output_dir) {
    printer_.set_indent_char(' ');
    printer_.set_indent_step(4);
  }

  virtual std::string
  output_filename() final {
    std::stringstream str;
    str << output_dir << "/";
    for (auto i = 0u; i < package_parts().size(); ++i) {
      str << package_parts()[i] << "/";
    }
    str << input_filename_without_ext() + "_smf_client.py";
    return str.str();
  }

  virtual std::optional<std::string>
  gen() final {
    generate_header_prologue();
    generate_header_includes();
    generate_header_services();
    // for Python make sure that the directories exist
    boost::filesystem::create_directories(
      boost::algorithm::join(package_parts(), "/"));
    return save_conents_to_file();
  }

 private:
  void generate_header_prologue();
  void generate_header_includes();
  void generate_header_services();
};

}  // namespace smf_gen
