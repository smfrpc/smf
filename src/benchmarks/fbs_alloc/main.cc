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
#include <core/print.hh>
#include <core/sstring.hh>
#include <core/temporary_buffer.hh>

#include "kv_generated.h"
#include "smf/native_type_utils.h"

static inline kvpairT
gen_kv(uint32_t sz) {
  kvpairT ret;
  ret.key.resize(sz);
  ret.value.resize(sz);
  std::fill(ret.key.begin(), ret.key.end(), 'x');
  std::fill(ret.value.begin(), ret.value.end(), 'x');
  return ret;
}

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
    benchmark::DoNotOptimize(mem);
  }
}
BENCHMARK(BM_alloc_simple)
  ->Args({1 << 1, 1 << 1})
  ->Args({1 << 2, 1 << 2})
  ->Args({1 << 4, 1 << 4})
  ->Args({1 << 8, 1 << 8})
  ->Args({1 << 12, 1 << 12})
  ->Args({1 << 16, 1 << 16})
  ->Args({1 << 18, 1 << 18});
// OOMs after this! ->Args({1 << 20, 1 << 20});

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
      "ERRNO: {}, Bad aligned allocation of {} with alignment: {}", r,
      bdr.GetSize(), 8);
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
  ->Args({1 << 18, 1 << 18});

static void
BM_alloc_hybrid(benchmark::State &state) {
  for (auto _ : state) {
    state.PauseTiming();
    auto kv = gen_kv(state.range(0));
    state.ResumeTiming();
    auto buf = smf::native_table_as_buffer<kvpair>(kv);
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
  ->Args({1 << 18, 1 << 18});


BENCHMARK_MAIN();
