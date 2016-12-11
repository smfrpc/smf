#pragma once
// seastar
#include <core/fstream.hh>
// generated
#include "flatbuffers/wal_generated.h"
// smf
#include "filesystem/wal_reader_node.h"
#include "filesystem/wal_opts.h"
#include "log.h"

namespace smf {


/// \brief - given a starting directory and an epoch to read
/// it will iterate through the records on the file.
///
/// - in design
class wal_reader {
  public:
  struct offset_range {
    uint64_t begin{0};
    uint64_t end{0};
    bool operator==(const offset_range &o) const {
      return begin == o.begin && end == o.end;
    }
  };
  struct offset_range_hasher {
    size_t operator()(const offset_range &o) const { return o.begin; }
  };


  explicit wal_reader(const wal_opts *o) : opts_(DTHROW_IFNULL(o)) {}
  wal_reader(wal_reader &&o) noexcept : opts_(DTHROW_IFNULL(o.opts_)),
                                        readers_(std::move(o.readers_)) {}

  /// brief - returns the next record in the log
  future<wal_opts::maybe_buffer> get(uint64_t epoch) {
    return make_ready_future<wal_opts::maybe_buffer>();
  }

  future<> close();
  future<> open();
  ~wal_reader();

  private:
  const wal_opts *opts_;
  // datastructure for in memory
  // these are the datastructures for disk
  std::unordered_map<offset_range,
                     std::unique_ptr<wal_reader_node>,
                     offset_range_hasher> readers_;
};

} // namespace smf
