// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <utility>
// third party
#include <core/file.hh>
// smf
#include "filesystem/wal_name_extractor_utils.h"

namespace smf {
struct wal_file_walker {
  explicit wal_file_walker(seastar::file d)
    : directory(std::move(d))
    , listing(directory.list_directory(
        [this](seastar::directory_entry de) { return dir_visit(de); })) {}

  virtual ~wal_file_walker() {}

  virtual seastar::future<> visit(seastar::directory_entry wal_file_entry) = 0;

  virtual seastar::future<> dir_visit(seastar::directory_entry de) final {
    if (de.type /*optional type*/
        && de.type == seastar::directory_entry_type::regular
        && wal_name_extractor_utils::is_wal_file_name(de.name)) {
      return visit(std::move(de));
    }
    return seastar::make_ready_future<>();
  }

  seastar::future<> done() { return listing.done(); }

  seastar::file                                   directory;
  seastar::subscription<seastar::directory_entry> listing;
};

}  // namespace smf
