include(smf)
# should be first so we can clear the world after it
add_subdirectory(third_party)

if(SMF_BUILD_COMPILER)
  add_subdirectory(smfc)
endif()

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
