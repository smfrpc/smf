#pragma once
#include <memory>
// smf
#include "filesystem/wal_reader_node.h"

namespace smf {
struct wal_reader_offset_range {
  wal_reader_offset_range(sstring filename, reader_stats *s) {
    node = std::make_unique<wal_reader_node>(filename, s);
  }
  wal_reader_offset_range(wal_reader_offset_range &&o)
    : begin(o.begin), end(o.end), node(std::move(node)) {}

  future<> open() {
    return node->open().then([this] {
      begin = node->starting_offset();
      end = begin + node->file_size();
      return make_ready_future<>();
    });
  }
  future<> close() { return node->close(); }
  uint64_t begin{0};
  uint64_t end{0};
  std::unique_ptr<wal_reader_node> node;
};
} // namespace smf
