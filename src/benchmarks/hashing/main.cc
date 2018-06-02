// Copyright 2018 Alexander Gallego
//

#include <cstring>
#include <memory>
#include <random>
#include <thread>
#include <unordered_map>

#include <benchmark/benchmark.h>
#include <bytell_hash_map.hpp>
#include <flat_hash_map.hpp>


struct simple_random {
  simple_random() {
    std::random_device rd;
    rand.seed(rd());
  }

  std::mt19937 rand;
  // by default range [0, MAX]
  std::uniform_int_distribution<uint64_t> dist;

  inline uint64_t
  next() {
    return dist(rand);
  }
};

static void
BM_flat(benchmark::State &state) {
  simple_random r;
  ska::flat_hash_map<uint64_t, uint64_t> map;
  for (auto i = 0; i < state.range(0); ++i) {
    auto k = r.next();
    map.insert({k, k});
  }
  for (auto _ : state) {
    uint64_t key;
    state.PauseTiming();
    key = r.next() % state.range(0);
    state.ResumeTiming();
    benchmark::DoNotOptimize(map.find(key));
  }
}
BENCHMARK(BM_flat)
  ->Args({200, 200})
  ->Args({2000, 2000})
  ->Args({20000, 20000})
  ->Args({200000, 200000})
  ->Args({2000000, 2000000})
  ->Args({20000000, 20000000});

static void
BM_bytell(benchmark::State &state) {
  simple_random r;
  ska::bytell_hash_map<uint64_t, uint64_t> map;
  for (auto i = 0; i < state.range(0); ++i) {
    auto k = r.next();
    map.insert({k, k});
  }
  for (auto _ : state) {
    uint64_t key;
    state.PauseTiming();
    key = r.next() % state.range(0);
    state.ResumeTiming();
    benchmark::DoNotOptimize(map.find(key));
  }
}
BENCHMARK(BM_bytell)
  ->Args({200, 200})
  ->Args({2000, 2000})
  ->Args({20000, 20000})
  ->Args({200000, 200000})
  ->Args({2000000, 2000000})
  ->Args({20000000, 20000000});

static void
BM_std_unordered(benchmark::State &state) {
  simple_random r;
  std::unordered_map<uint64_t, uint64_t> map;
  for (auto i = 0; i < state.range(0); ++i) {
    auto k = r.next();
    map.insert({k, k});
  }
  for (auto _ : state) {
    uint64_t key;
    state.PauseTiming();
    key = r.next() % state.range(0);
    state.ResumeTiming();
    benchmark::DoNotOptimize(map.find(key));
  }
}
BENCHMARK(BM_std_unordered)
  ->Args({200, 200})
  ->Args({2000, 2000})
  ->Args({20000, 20000})
  ->Args({200000, 200000})
  ->Args({2000000, 2000000})
  ->Args({20000000, 20000000});

BENCHMARK_MAIN();
