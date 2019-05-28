// Copyright 2018 SMF Authors
//

#pragma once
#include <map>
#include <memory>
#include <set>
#include <string>

#include <boost/filesystem.hpp>
#include <flatbuffers/flatbuffers.h>

#include "smf_printer.h"
#include "smf_service.h"
#include "smf/std-compat.h"

namespace smf_gen {
class generator {
 public:
  generator(const flatbuffers::Parser &p, const std::string &ifname,
            const std::string &out_dir)
    : parser(p), input_filename(ifname), output_dir(out_dir) {
    for (const auto *s : parser.services_.vec) {
      if (s->generated) {
        // True for transitive flatbuffer files.
        // If you have a.fbs include b.fbs, then the services on b.fbs
        // will be marked as generated
        continue;
      }
      services_.emplace_back(std::make_unique<smf_service>(s));
    }
  }
  virtual ~generator() = default;

  virtual std::string output_filename() = 0;
  virtual smf::compat::optional<std::string> gen() = 0;

  virtual const std::string &
  contents() final {
    return printer_.contents();
  }

  const flatbuffers::Parser &parser;
  const std::string &input_filename;
  const std::string &output_dir;

  virtual const std::map<std::string, std::set<std::string>> &
  fbs_files_included_per_file() const final {
    return parser.files_included_per_file_;
  }
  virtual const std::vector<std::string> &
  native_included_files() const final {
    return parser.native_included_files_;
  }
  virtual const std::map<std::string, std::string> &
  included_files() const final {
    return parser.included_files_;
  };

  virtual const std::vector<std::unique_ptr<smf_service>> &
  services() final {
    return services_;
  }

  virtual std::string
  package() const final {
    return parser.current_namespace_->GetFullyQualifiedName("");
  }

  virtual std::vector<std::string>
  package_parts() const final {
    return parser.current_namespace_->components;
  }

  virtual std::string
  input_filename_without_path() const final {
    return flatbuffers::StripPath(input_filename);
  }

  virtual std::string
  input_filename_without_ext() const final {
    return flatbuffers::StripExtension(input_filename_without_path());
  }

  virtual smf::compat::optional<std::string>
  save_conents_to_file() {
    auto outname = output_filename();
    if (boost::filesystem::exists(outname)) {
      boost::filesystem::remove(outname);
    }
    if (!flatbuffers::SaveFile(outname.c_str(), printer_.contents(), false)) {
      return "Could not create filename: " + outname;
    }
    return smf::compat::nullopt;
  }

 protected:
  smf_printer printer_;

 private:
  std::vector<std::unique_ptr<smf_service>> services_;
};
}  // namespace smf_gen
