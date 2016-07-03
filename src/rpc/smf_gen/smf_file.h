#pragma once
#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>
#include <flatbuffers/idl.h>

// smf_gen
#include "rpc/smf_gen/smf_printer.h"
#include "rpc/smf_gen/smf_service.h"

namespace smf_gen {
class smf_file {
  public:
  smf_file(const flatbuffers::Parser &parser, const std::string &file_name)
    : parser_(parser), file_name_(file_name) {}

  std::string filename() const { return file_name_; }
  std::string filename_without_path() const {
    return flatbuffers::StripPath(file_name_);
  }
  std::string filename_without_ext() const {
    return flatbuffers::StripExtension(filename_without_path());
  }
  std::string message_header_ext() const { return "_generated.h"; }
  std::string service_header_ext() const { return ".smf.fb.h"; }
  std::string package() const {
    return parser_.namespaces_.back()->GetFullyQualifiedName("");
  }
  std::vector<std::string> package_parts() const {
    return parser_.namespaces_.back()->components;
  }
  int service_count() const {
    return static_cast<int>(parser_.services_.vec.size());
  };
  std::unique_ptr<const smf_service> service(int i) const {
    return std::unique_ptr<const smf_service>(
      new smf_service(parser_.services_.vec[i]));
  }
  std::unique_ptr<smf_printer> create_printer(std::string *str) const {
    return std::unique_ptr<smf_printer>(new smf_printer(str));
  }

  private:
  const flatbuffers::Parser &parser_;
  const std::string &file_name_;
};
} // namespace
