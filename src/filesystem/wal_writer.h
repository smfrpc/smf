#pragma once
#include <memory>
// smf
#include "filesystem/wal_writer_node.h"

namespace smf {
class wal_writer {
  public:
  explicit wal_writer(sstring _directory) : directory(_directory) {}
  wal_writer(const wal_writer &o) = delete;
  wal_writer(wal_writer &&o)
    : directory(o.directory), writer_(std::move(o.writer_)) {}

  future<> open();
  future<> append(temporary_buffer<char> &&buf);
  future<> close();
  const sstring directory;

  private:
  std::unique_ptr<wal_writer_node> writer_ = nullptr;
};
}
