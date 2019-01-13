find_package(benchmark REQUIRED)
include(smfc_generator)
smfc_gen(
  CPP
  TARGET_NAME kv_fbs
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/kv.fbs)
smf_test(
  BENCHMARK_TEST
  BINARY_NAME fbsalloc
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cc
    ${kv_fbs}
  SOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  TIMEOUT 420 # on my laptop test takes 320 secs
  INCLUDES ${CMAKE_CURRENT_BINARY_DIR}
  LIBRARIES benchmark::benchmark smf
  )
