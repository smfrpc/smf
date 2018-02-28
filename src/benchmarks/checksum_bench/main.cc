// Copyright 2018 Alexander Gallego
//

#include <cstring>
#include <memory>
#include <thread>

#include <benchmark/benchmark.h>

#include "hashing/hashing_utils.h"

using namespace smf;  // NOLINT

static constexpr uint32_t kPayloadSize = 1 << 29;
static char               kPayload[kPayloadSize]{};

static void
BM_hash32(benchmark::State &state) {
  for (auto _ : state) {
    state.PauseTiming();
    std::memset(kPayload, 'x', state.range(0));
    state.ResumeTiming();
    benchmark::DoNotOptimize(xxhash_32(kPayload, state.range(0)));
  }
}
BENCHMARK(BM_hash32)
  ->Args({1 << 16, 1 << 16})
  ->Args({1 << 20, 1 << 20})
  ->Args({1 << 29, 1 << 29});


static void
BM_hash64(benchmark::State &state) {
  for (auto _ : state) {
    state.PauseTiming();
    std::memset(kPayload, 'x', state.range(0));
    state.ResumeTiming();
    benchmark::DoNotOptimize(std::numeric_limits<uint32_t>::max() &
                             xxhash_64(kPayload, state.range(0)));
  }
}
BENCHMARK(BM_hash64)
  ->Args({1 << 16, 1 << 16})
  ->Args({1 << 20, 1 << 20})
  ->Args({1 << 29, 1 << 29});


BENCHMARK_MAIN();
