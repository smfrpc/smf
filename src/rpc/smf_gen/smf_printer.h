// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <map>
#include <string>
namespace smf_gen {
class smf_printer {
 public:
  explicit smf_printer(std::string *str)
    : str_(str), escape_char_('$'), indent_(0) {}

  void print(const std::map<std::string, std::string> &vars,
             const char *string_template) {
    std::string s = string_template;
    // Replace any occurrences of strings in "vars" that are surrounded
    // by the escape character by what they're mapped to.
    size_t pos;
    while ((pos = s.find(escape_char_)) != std::string::npos) {
      // Found an escape char, must also find the closing one.
      size_t pos2 = s.find(escape_char_, pos + 1);
      // If placeholder not closed, ignore.
      if (pos2 == std::string::npos)
        break;
      auto it = vars.find(s.substr(pos + 1, pos2 - pos - 1));
      // If unknown placeholder, ignore.
      if (it == vars.end())
        break;
      // Subtitute placeholder.
      s.replace(pos, pos2 - pos + 1, it->second);
    }
    print(s.c_str());
  }

  void print(const char *s) {
    // Add this string, but for each part separated by \n, add indentation.
    for (;;) {
      // Current indentation.
      str_->insert(str_->end(), indent_ * 2, ' ');
      // See if this contains more than one line.
      const char *lf = strchr(s, '\n');
      if (lf) {
        (*str_) += std::string(s, lf + 1);
        s = lf + 1;
        if (!*s)
          break;  // Only continue if there's more lines.
      } else {
        (*str_) += s;
        break;
      }
    }
  }

  void indent() { indent_++; }
  void outdent() {
    indent_--;
    assert(indent_ >= 0);
  }

 private:
  std::string *str_;
  char         escape_char_;
  int          indent_;
};
}  // namespace smf_gen
