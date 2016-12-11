#include "filesystem/wal_impl_with_cache.h"

namespace smf {
wal_impl_with_cache::wal_impl_with_cache(wal_opts _opts)
  : wal(std::move(_opts)) {
  rdr_ = std::make_unique<wal_reader>(opts.directory);
  wtr_ = std::make_unique<wal_writer>(opts.directory);
  cache_ = std::make_unique<wal_mem_cache>(opts.cache_size);
}

future<> wal_impl_with_cache::append(temporary_buffer<char> &&buf) {}
future<> wal_impl_with_cache::invalidate(uint64_t epoch) {}
future<wal_opts::maybe_buffer> wal_impl_with_cache::get(uint64_t epoch) {}
};
} // namespace smf
