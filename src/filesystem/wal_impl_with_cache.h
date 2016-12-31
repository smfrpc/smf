#pragma once
#include "filesystem/wal.h"
#include "filesystem/wal_mem_cache.h"
#include "filesystem/wal_reader.h"
#include "filesystem/wal_writer.h"
namespace smf {
class wal_impl_with_cache : public wal {
  public:
  explicit wal_impl_with_cache(wal_opts _opts);

  virtual future<uint64_t> append(wal_write_request req) override final;

  virtual future<> invalidate(uint64_t epoch) override final;

  virtual future<wal_opts::maybe_buffer>
  get(wal_read_request req) override final;

  private:
  std::unique_ptr<wal_writer> wtr_ = nullptr;
  std::unique_ptr<wal_reader> rdr_ = nullptr;
  std::unique_ptr<wal_mem_cache> cache_ = nullptr;
};
}
