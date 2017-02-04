// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
// std
#include <utility>
// smf
#include "filesystem/wal_file_walker.h"
#include "filesystem/wal_name_extractor_utils.h"

namespace smf {
struct wal_head_file_max_comparator {
  bool operator()(const sstring &current, const sstring &new_file) {
    auto current_epoch = extract_epoch(current);
    auto new_epoch     = extract_epoch(new_file);
    return current_epoch <= new_epoch;
  }
};
struct wal_head_file_min_comparator {
  bool operator()(const sstring &current, const sstring &new_file) {
    auto current_epoch = extract_epoch(current);
    auto new_epoch     = extract_epoch(new_file);
    return current_epoch >= new_epoch;
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
template <typename Comparator> struct wal_head_file_functor : wal_file_walker {
  explicit wal_head_file_functor(file dir) : wal_file_walker(std::move(dir)) {}
  future<> visit(directory_entry de) final {
    if (comparator(last_file, de.name)) {
      last_file = de.name;
    }
    return make_ready_future<>();
  }
  sstring    last_file;
  Comparator comparator{};
};

using wal_head_file_max_functor =
  wal_head_file_functor<wal_head_file_max_comparator>;

using wal_head_file_min_functor =
  wal_head_file_functor<wal_head_file_min_comparator>;
}  // namespace smf
