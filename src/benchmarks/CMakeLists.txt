set(BENCH_ROOT ${PROJECT_SOURCE_DIR}/src/benchmarks)

smf_test(
  BENCHMARK_TEST
  BINARY_NAME checksum
  SOURCES ${BENCH_ROOT}/checksum_bench/main.cc
  SOURCE_DIRECTORY ${BENCH_ROOT}/checksum_bench
  LIBRARIES benchmark smf
  )
