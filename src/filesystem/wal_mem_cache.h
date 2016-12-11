#pragma once
namespace smf {
class wal_mem_cache {
  public:
  /// brief - caches at the virtual offset the buffer
  /// it is different from the absolute buffer, because this is after
  /// `buf` is written to disk, which might be compressed
  future<> put(uint64_t virtual_offset, temporary_buffer<char> &&buf);
  /// brief - invalidates
  future<> invalidate(uint64_t virtual_offset);

  private:
};
} // namespace smf
