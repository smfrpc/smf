// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <fstream>
#include <iostream>
#include <string>

#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/idl.h>
#include <flatbuffers/util.h>
#include <glog/logging.h>

#include "generator.h"

namespace smf_gen {

class cpp_generator : public generator {
 public:
  cpp_generator(const flatbuffers::Parser &p, const std::string &ifname,
                const std::string &output_dir)
    : generator(p, ifname, output_dir) {}
  virtual ~cpp_generator() = default;

  virtual std::string
  output_filename() final {
    return output_dir + "/" + input_filename_without_ext() + ".smf.fb.h";
  }

  virtual std::optional<std::string>
  gen() final {
    generate_header_prologue();
    generate_header_prologue_includes();

    generate_header_prologue_namespace();
    generate_header_prologue_forward_decl();
    generate_header_epilogue_namespace();

    generate_header_prologue_forward_decl_external();

    generate_header_prologue_namespace();
    generate_header_services();

    generate_header_epilogue_namespace();
    generate_header_epilogue();

    return save_conents_to_file();
  }

  std::string
  message_header_ext() const {
    return "_generated.h";
  }

 private:
  void generate_header_prologue();
  void generate_header_prologue_includes();
  void generate_header_prologue_forward_decl();
  void generate_header_prologue_forward_decl_external();
  void generate_header_prologue_namespace();
  void generate_header_services();
  void generate_header_epilogue();
  void generate_header_epilogue_namespace();
};

}  // namespace smf_gen
