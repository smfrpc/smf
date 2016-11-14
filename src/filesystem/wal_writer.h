#pragma once
#include <memory>
// smf
#include "filesystem/wal_writer_node.h"

namespace smf {
class wal_writer {
  public:
  // maybe expose the prefix of the files? i.e.: "smf_`epoch'_`time'.wal"
  // - the prefix would be `smf'
  explicit wal_writer(sstring _directory) : directory(_directory) {}
  wal_writer(const wal_writer &o) = delete;
  wal_writer(wal_writer &&o)
    : directory(o.directory), current_(std::move(o.current_)) {}

  future<> open();
  future<> append(temporary_buffer<char> &&buf);

  const sstring directory;

  private:
  std::unique_ptr<wal_writer_node> current_ = nullptr;
};
}
