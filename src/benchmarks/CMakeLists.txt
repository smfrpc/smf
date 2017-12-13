set(BENCH_ROOT ${PROJECT_SOURCE_DIR}/src/benchmarks)

smf_test(
  BENCHMARK_TEST
  BINARY_NAME topic_partition_hasher
  SOURCES ${BENCH_ROOT}/topic_partition_hashing/main.cc
  SOURCE_DIRECTORY ${BENCH_ROOT}/topic_partition_hashing
  LIBRARIES benchmark smf_utils smf_hashing
  )
smf_test(
  BENCHMARK_TEST
  BINARY_NAME semaphore_load
  SOURCES ${BENCH_ROOT}/core_balance/main.cc
  SOURCE_DIRECTORY ${BENCH_ROOT}/core_balance
  LIBRARIES benchmark smf_hashing fmt seastar
  )
