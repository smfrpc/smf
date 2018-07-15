// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <map>
#include <string>

namespace smf_gen {
class smf_printer {
 public:
  explicit smf_printer(char escape = '$') : escape_char(escape) {}

  void
  print(const std::map<std::string, std::string> &vars,
        const char *string_template) {
    std::string s = string_template;
    // Replace any occurrences of strings in "vars" that are surrounded
    // by the escape character by what they're mapped to.
    size_t pos;
    while ((pos = s.find(escape_char)) != std::string::npos) {
      // Found an escape char, must also find the closing one.
      size_t pos2 = s.find(escape_char, pos + 1);
      // If placeholder not closed, ignore.
      if (pos2 == std::string::npos) break;
      auto it = vars.find(s.substr(pos + 1, pos2 - pos - 1));
      // If unknown placeholder, ignore.
      if (it == vars.end()) break;
      // Subtitute placeholder.
      s.replace(pos, pos2 - pos + 1, it->second);
    }
    print(s.c_str());
  }

  void
  print(const char *s) {
    // Add this string, but for each part separated by \n, add indentation.
    for (;;) {
      // Current indentation.
      out_.insert(out_.end(), indent_ * indent_step_, indent_char_);
      // See if this contains more than one line.
      const char *lf = strchr(s, '\n');
      if (lf) {
        out_ += std::string(s, lf + 1);
        s = lf + 1;
        if (!*s) break;  // Only continue if there's more lines.
      } else {
        out_ += s;
        break;
      }
    }
  }

  void
  indent() {
    indent_++;
  }
  void
  outdent() {
    indent_--;
    assert(indent_ >= 0);
  }

  const char escape_char;

  const std::string &
  contents() const {
    return out_;
  }

  void
  set_indent_char(char i) {
    indent_char_ = i;
  }
  void
  set_indent_step(uint16_t step) {
    indent_step_ = step;
  }

 private:
  std::string out_;
  int32_t indent_{0};
  char indent_char_ = ' ';
  uint16_t indent_step_ = 2;
};
}  // namespace smf_gen
