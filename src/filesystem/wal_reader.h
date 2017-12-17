// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <list>
#include <memory>
#include <utility>

#include <core/sstring.hh>

#include "filesystem/wal_file_walker.h"
#include "filesystem/wal_reader_node.h"
#include "filesystem/wal_requests.h"
#include "flatbuffers/wal_generated.h"

namespace smf {
class wal_reader;
struct wal_reader_visitor : wal_file_walker {
  wal_reader_visitor(wal_reader *r, seastar::file dir);
  seastar::future<> visit(seastar::directory_entry wal_file_entry) final;
  wal_reader *reader;
};

/// \brief - given a starting directory and an epoch to read
/// it will iterate through the records on the file.
///
/// - in design
class wal_reader {
 private:
  struct reader_bucket : public boost::intrusive::set_base_hook<>,
                         public boost::intrusive::unordered_set_base_hook<> {
    explicit reader_bucket(std::unique_ptr<wal_reader_node> n)
      : node(std::move(n)) {}
    std::unique_ptr<wal_reader_node> node;
  };
  struct reader_bucket_key {
    typedef int64_t type;
    const type &operator()(const reader_bucket &v) const {
      return v.node->starting_epoch;
    }
  };
  using intrusive_key = boost::intrusive::key_of_value<reader_bucket_key>;
  using intrusive_map = boost::intrusive::set<reader_bucket, intrusive_key>;
  static_assert(std::is_same<intrusive_map::key_type, int64_t>::value,
                "bad key for intrusive map");

 public:
  explicit wal_reader(seastar::sstring workdir);
  wal_reader(wal_reader &&o) noexcept;
  ~wal_reader();
  SMF_DISALLOW_COPY_AND_ASSIGN(wal_reader);

  seastar::future<> open();
  seastar::future<> close();
  /// brief - returns the next record in the log
  seastar::future<wal_read_reply> get(wal_read_request req);

  /// \brief this is risky and is for performance reasons
  /// we do not want to fs::stat files all the time.
  /// This recomputes offsets of files that we want to read
  void update_file_size_by(int64_t node_epoch, uint64_t delta);

  const seastar::sstring work_directory;

 private:
  friend wal_reader_visitor;
  seastar::future<> monitor_files(seastar::directory_entry wal_file_entry);

  std::list<reader_bucket>            allocated_;
  intrusive_map                       buckets_;
  std::unique_ptr<wal_reader_visitor> fs_observer_;
};

}  // namespace smf
