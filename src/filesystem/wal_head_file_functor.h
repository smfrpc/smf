#pragma once
#include "filesystem/wal_name_parser.h"

namespace smf {
struct wal_head_file_max_comparator {
  bool operator()(const sstring &a, const sstring &b) { return a < b; }
};
struct wal_head_file_min_comparator {
  bool operator()(const sstring &a, const sstring &b) {
    return a != b && !(a < b);
  }
};

/// \brief used from a subscriber to find the last file on the file system
/// according to our naming scheme of `file`_`size`_`date`, i.e.:
/// for a given directory.
/// Given `/tmp/smf/wal` as a directory, we'll find files that looks like this
/// "smf_2345234_23421234.wal"
///
/// example:
///       auto l = make_lw_shared<file_visitor>(std::move(f));
///       return l->close().then([l] {
///            LOG_INFO("Last file found: {}", l->file_name);
///            return make_ready_future<>();
///       });
///
template <typename Comparator> struct wal_head_file_functor {
  wal_head_file_functor(file _f, sstring _prefix = "smf")
    : f(std::move(_f))
    , name_parser(std::move(_prefix))
    , listing(
        f.list_directory([this](directory_entry de) { return visit(de); })) {}

  future<> visit(directory_entry de) {
    if(de.type && de.type == directory_entry_type::regular) {
      // operator >(const T&) is not defined, but operator <(const T&) is
      if(name_parser(de.name) && comparator(last_file, de.name)) {
        last_file = de.name;
      }
    }
    return make_ready_future<>();
  }
  future<> close() { return listing.done(); }

  file f;
  wal_name_parser name_parser;
  sstring last_file;
  subscription<directory_entry> listing;
  Comparator comparator{};
};

using wal_head_file_max_functor =
  wal_head_file_functor<wal_head_file_max_comparator>;

using wal_head_file_min_functor =
  wal_head_file_functor<wal_head_file_min_comparator>;
} // namespace smf
