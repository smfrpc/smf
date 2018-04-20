#pragma once
#include <sstream>

#include "generator.h"

namespace smf_gen {

class go_generator : public generator {
 public:
  go_generator(const flatbuffers::Parser &p,
    const std::string &ifname,
    const std::string &output_dir)
    : generator(p, ifname, output_dir) {}
  virtual ~go_generator() = default;

  virtual std::string
  output_filename() final {
    std::stringstream str;
    str << output_dir << "/";
    for (auto i = 0u; i < package_parts().size(); ++i) {
      str << package_parts()[i] << "/";
    }
    str << input_filename_without_ext() + ".smf.fb.go";
    return str.str();
  }

  virtual std::experimental::optional<std::string>
  gen() final {
    return std::experimental::nullopt;
  }
};

}  // namespace smf_gen
