#pragma once
// seastar
#include <core/fstream.hh>
// generated
#include "flatbuffers/wal_generated.h"

namespace smf {
// TODO(agallego) - missing stats


class wal_reader_node {
  public:
  wal_reader_node(sstring _filename, const wal_opts *o)
    : filename(_filename), opts(DTHROW_IFNULL(o)) {}

  /// \brief flushes the file before closing
  future<> close();
  future<> open();
  future<wal_opts::maybe_buffer> get(uint64_t epoch) {
        return make_ready_future<wal_opts::maybe_buffer>();
  }
  ~wal_reader_node();

  const sstring filename;
  const wal_opts *opts;


  private:
  input_stream<char> ifstream_;
};

} // namespace smf
