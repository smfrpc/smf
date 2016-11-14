#pragma once
#include "filesystem/wal_name_parser.h"

// deleteme
#include "log.h"

namespace smf {

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
struct wal_head_file_functor {
  wal_head_file_functor(file _f, sstring _prefix = "smf")
    : f(std::move(_f))
    , name_parser(std::move(_prefix))
    , listing(
        f.list_directory([this](directory_entry de) { return visit(de); })) {}

  future<> visit(directory_entry de) {
    if(de.type && de.type == directory_entry_type::regular) {

      LOG_INFO("deleteme: found file: {}", de.name);

      // operator >(const T&) is not defined, but operator <(const T&) is
      if(name_parser(de.name) && last_file < de.name) {
        last_file = de.name;
        LOG_INFO("deleteme Parsing directory, found a match {}", last_file);
      }
    }
    return make_ready_future<>();
  }

  future<> close() { return listing.done(); }


  file f;
  wal_name_parser name_parser;
  sstring last_file;
  subscription<directory_entry> listing;
};
}
