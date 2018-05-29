include(${PROJECT_SOURCE_DIR}/CMake/smf.cmake)
# should be first so we can clear the world after it
add_subdirectory(third_party)

if(SMF_BUILD_COMPILER)
  add_subdirectory(smfc)
endif()

add_subdirectory(core)

if(SMF_ENABLE_BENCHMARK_TESTS)
  add_subdirectory(benchmarks)
endif(SMF_ENABLE_BENCHMARK_TESTS)

if(SMF_ENABLE_INTEGRATION_TESTS)
  add_subdirectory(integration_tests)
endif(SMF_ENABLE_INTEGRATION_TESTS)

# add go
add_subdirectory(go)
