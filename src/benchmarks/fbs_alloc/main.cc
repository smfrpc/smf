// Copyright 2018 SMF Authors
//

//
// Note
// Important as we upgrade / change the flatbuffers versions or their internal
// API changes
// The result is wrapped up in the native_type_utils.h
#include <cstring>
#include <memory>
#include <thread>

#include <benchmark/benchmark.h>
#include <seastar/core/print.hh>
#include <seastar/core/sstring.hh>
#include <seastar/core/temporary_buffer.hh>

#include "kv_generated.h"
#include "smf/native_type_utils.h"

static inline kvpairT
gen_kv(uint32_t sz) {
  kvpairT ret;
  ret.key.resize(sz, 'x');
  ret.value.resize(sz, 'y');
  return ret;
}

static void
BM_malloc_base(benchmark::State &state) {
  for (auto _ : state) {
    auto buf = reinterpret_cast<char *>(std::malloc(2 * state.range(0)));
    std::memset(buf, 'x', 2 * state.range(0));
    benchmark::DoNotOptimize(buf);
    std::free(buf);
  }
}
BENCHMARK(BM_malloc_base)
  ->Args({1 << 1, 1 << 1})
  ->Args({1 << 2, 1 << 2})
  ->Args({1 << 4, 1 << 4})
  ->Args({1 << 8, 1 << 8})
  ->Args({1 << 12, 1 << 12})
  ->Args({1 << 16, 1 << 16})
  ->Args({1 << 18, 1 << 18})
  ->Threads(1);

static void
BM_alloc_simple(benchmark::State &state) {
  for (auto _ : state) {
    state.PauseTiming();
    auto kv = gen_kv(state.range(0));
    state.ResumeTiming();

    // build it!
    flatbuffers::FlatBufferBuilder bdr;
    bdr.Finish(kvpair::Pack(bdr, &kv, nullptr));
    auto mem = bdr.Release();
    auto ptr = reinterpret_cast<char *>(mem.data());
    auto sz = mem.size();
    auto buf = seastar::temporary_buffer<char>(
      ptr, sz, seastar::make_object_deleter(std::move(mem)));
    benchmark::DoNotOptimize(buf);
  }
}
BENCHMARK(BM_alloc_simple)
  ->Args({1 << 1, 1 << 1})
  ->Args({1 << 2, 1 << 2})
  ->Args({1 << 4, 1 << 4})
  ->Args({1 << 8, 1 << 8})
  ->Args({1 << 12, 1 << 12})
  ->Args({1 << 16, 1 << 16})
  ->Args({1 << 18, 1 << 18})
  ->Threads(1);

static void
BM_alloc_thread_local(benchmark::State &state) {
  static thread_local flatbuffers::FlatBufferBuilder bdr(1024);
  for (auto _ : state) {
    state.PauseTiming();
    auto kv = gen_kv(state.range(0));
    state.ResumeTiming();

    // key operations
    bdr.Clear();
    bdr.Finish(kvpair::Pack(bdr, &kv, nullptr));
    // ned key operations

    // std::cout << "Size to copy: " << bdr.GetSize() << std::endl;

    void *ret = nullptr;
    auto r = posix_memalign(&ret, 8, bdr.GetSize());
    if (r == ENOMEM) {
      throw std::bad_alloc();
    } else if (r == EINVAL) {
      throw std::runtime_error(seastar::sprint(
        "Invalid alignment of %d; allocating %d bytes", 8, bdr.GetSize()));
    }
    DLOG_THROW_IF(r != 0,
                  "ERRNO: {}, Bad aligned allocation of {} with alignment: {}",
                  r, bdr.GetSize(), 8);
    std::memcpy(ret, reinterpret_cast<const char *>(bdr.GetBufferPointer()),
                bdr.GetSize());
    benchmark::DoNotOptimize(ret);
    std::free(ret);
  }
}
BENCHMARK(BM_alloc_thread_local)
  ->Args({1 << 1, 1 << 1})
  ->Args({1 << 2, 1 << 2})
  ->Args({1 << 4, 1 << 4})
  ->Args({1 << 8, 1 << 8})
  ->Args({1 << 12, 1 << 12})
  ->Args({1 << 16, 1 << 16})
  ->Args({1 << 18, 1 << 18})
  ->Threads(1);

template <typename RootType>
seastar::temporary_buffer<char>
hybrid(const typename RootType::NativeTableType &t) {
  static thread_local auto builder =
    std::make_unique<flatbuffers::FlatBufferBuilder>();

  auto &bdr = *builder.get();
  bdr.Clear();
  bdr.Finish(RootType::Pack(bdr, &t, nullptr));

  if (SMF_UNLIKELY(bdr.GetSize() > 2048)) {
    auto mem = bdr.Release();
    auto ptr = reinterpret_cast<char *>(mem.data());
    auto sz = mem.size();

    // fix the original builder
    builder = std::make_unique<flatbuffers::FlatBufferBuilder>();

    return seastar::temporary_buffer<char>(
      ptr, sz, seastar::make_object_deleter(std::move(mem)));
  }

  // always allocate to the largest member 8-bytes
  void *ret = nullptr;
  auto r = posix_memalign(&ret, 8, bdr.GetSize());
  if (r == ENOMEM) {
    throw std::bad_alloc();
  } else if (r == EINVAL) {
    throw std::runtime_error(seastar::sprint(
      "Invalid alignment of %d; allocating %d bytes", 8, bdr.GetSize()));
  }
  DLOG_THROW_IF(r != 0,
                "ERRNO: {}, Bad aligned allocation of {} with alignment: {}", r,
                bdr.GetSize(), 8);
  seastar::temporary_buffer<char> retval(reinterpret_cast<char *>(ret),
                                         bdr.GetSize(),
                                         seastar::make_free_deleter(ret));
  std::memcpy(retval.get_write(),
              reinterpret_cast<const char *>(bdr.GetBufferPointer()),
              retval.size());
  return std::move(retval);
}

static void
BM_alloc_hybrid(benchmark::State &state) {
  for (auto _ : state) {
    state.PauseTiming();
    auto kv = gen_kv(state.range(0));
    state.ResumeTiming();
    auto buf = hybrid<kvpair>(kv);
    benchmark::DoNotOptimize(buf);
  }
}
BENCHMARK(BM_alloc_hybrid)
  ->Args({1 << 1, 1 << 1})
  ->Args({1 << 2, 1 << 2})
  ->Args({1 << 4, 1 << 4})
  ->Args({1 << 8, 1 << 8})
  ->Args({1 << 12, 1 << 12})
  ->Args({1 << 16, 1 << 16})
  ->Args({1 << 18, 1 << 18})
  ->Threads(1);

BENCHMARK_MAIN();
