include(smf)

add_subdirectory(smfc)
add_subdirectory(core)

if(SMF_ENABLE_BENCHMARK_TESTS)
  add_subdirectory(benchmarks)
endif()
if(SMF_ENABLE_INTEGRATION_TESTS)
  add_subdirectory(integration_tests)
endif()
if(SMF_ENABLE_UNIT_TESTS)
  add_subdirectory(tests)
endif()
