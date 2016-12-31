#pragma once
// std
#include <ostream>
// seastar
#include <core/future.hh>
#include <core/sstring.hh>
// smf
#include "filesystem/wal_opts.h"
#include "filesystem/wal_requests.h"

namespace smf {

/// brief - write ahead log
class wal {
  public:
  explicit wal(wal_opts o) : opts(std::move(o)) {}
  virtual ~wal() {}
  /// brief - used by seastar map-reduce
  inline const wal_opts &get_otps() const { return opts; }

  /// \brief returns starting offset off a successful write
  virtual future<uint64_t> append(wal_write_request req) = 0;
  virtual future<> invalidate(uint64_t epoch) = 0;
  virtual future<wal_opts::maybe_buffer> get(wal_read_request req) = 0;
  static std::unique_ptr<wal> make_wal(wal_type type, wal_opts opts);

  protected:
  wal_opts opts;
};
} // namespace smf
