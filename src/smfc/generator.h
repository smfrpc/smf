// Copyright 2018 SMF Authors
//

#pragma once

#include <experimental/optional>
#include <string>

#include <flatbuffers/flatbuffers.h>
#include <boost/filesystem.hpp>

#include "smf_printer.h"
#include "smf_service.h"

namespace smf_gen {
class generator {
 public:
  generator(const flatbuffers::Parser &p,
    const std::string &ifname,
    const std::string &out_dir)
    : parser(p), input_filename(ifname), output_dir(out_dir) {
    for (auto i = 0u; i < parser.services_.vec.size(); ++i) {
      services_.emplace_back(
        std::make_unique<smf_service>(parser.services_.vec[i]));
    }
  }
  virtual ~generator() = default;

  virtual std::string output_filename() = 0;
  virtual std::experimental::optional<std::string> gen() = 0;

  virtual const std::string &
  contents() final {
    return printer_.contents();
  }

  const flatbuffers::Parser &parser;
  const std::string &input_filename;
  const std::string &output_dir;


  virtual const std::vector<std::unique_ptr<smf_service>> &
  services() const final {
    return services_;
  }

  virtual std::string
  package() const final {
    return parser.namespaces_.back()->GetFullyQualifiedName("");
  }

  virtual std::vector<std::string>
  package_parts() const final {
    return parser.namespaces_.back()->components;
  }

  virtual std::string
  input_filename_without_path() const final {
    return flatbuffers::StripPath(input_filename);
  }

  virtual std::string
  input_filename_without_ext() const final {
    return flatbuffers::StripExtension(input_filename_without_path());
  }


  virtual std::experimental::optional<std::string>
  save_conents_to_file() {
    auto outname = output_filename();
    if (boost::filesystem::exists(outname)) {
      boost::filesystem::remove(outname);
    }
    if (!flatbuffers::SaveFile(outname.c_str(), printer_.contents(), false)) {
      return "Could not create filename: " + outname;
    }
    return std::experimental::nullopt;
  }


 protected:
  smf_printer printer_;

 private:
  std::vector<std::unique_ptr<smf_service>> services_;
};
}  // namespace smf_gen
