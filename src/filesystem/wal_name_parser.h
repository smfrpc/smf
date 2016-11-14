#pragma once
// third party
#include <boost/regex.hpp>
#include <core/sstring.hh>

namespace smf {
struct wal_name_parser {
  wal_name_parser(sstring _prefix = "smf") : prefix(_prefix) {
    for(char c : prefix) {
      if(c == '\\' || c == '(' || c == ')' || c == '[' || c == ']')
        throw std::runtime_error(
          "wal_name_parser cannot include a prefix with special chars");
    }
    sstring tmp = prefix + "_\\d+_[\\dT:-]+\\.wal";
    file_re = boost::regex(tmp.c_str());
  }
  const sstring prefix;
  boost::regex file_re;

  bool operator()(const sstring &filename) {
    return boost::regex_match(filename.c_str(), file_re);
  }
};


} // namespace smf
