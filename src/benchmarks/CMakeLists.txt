set(BENCH_ROOT ${PROJECT_SOURCE_DIR}/src/benchmarks)

smf_test(
  BENCHMARK_TEST
  BINARY_NAME checksum
  SOURCES ${BENCH_ROOT}/checksum_bench/main.cc
  SOURCE_DIRECTORY ${BENCH_ROOT}/checksum_bench
  LIBRARIES benchmark smf
  )
smf_test(
  BENCHMARK_TEST
  BINARY_NAME hash_table_comparison
  SOURCES ${BENCH_ROOT}/hashing/main.cc
  SOURCE_DIRECTORY ${BENCH_ROOT}/hashing
  TIMEOUT 420 # on my laptop test takes 320 secs
  INCLUDES ${PROJECT_SOURCE_DIR}/src/third_party/flat_hash_map
  LIBRARIES benchmark
  )
