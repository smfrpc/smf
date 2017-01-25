// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <utility>
// third party
#include <core/file.hh>
// smf
#include "filesystem/wal_name_parser.h"

namespace smf {
struct wal_file_walker {
  wal_file_walker(file d, sstring prefix)
    : directory(std::move(d))
    , name_parser(std::move(prefix))
    , listing(directory.list_directory(
        [this](directory_entry de) { return dir_visit(de); })) {}

  virtual future<> visit(directory_entry wal_file_entry) = 0;

  virtual future<> dir_visit(directory_entry de) final {
    if (de.type && de.type == directory_entry_type::regular) {
      // operator >(const T&) is not defined, but operator <(const T&) is
      if (name_parser(de.name)) {
        return visit(std::move(de));
      }
    }
    return make_ready_future<>();
  }

  future<> close() { return listing.done(); }

  file                          directory;
  wal_name_parser               name_parser;
  subscription<directory_entry> listing;
};

}  // namespace smf
