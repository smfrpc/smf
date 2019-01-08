if(SMF_ENABLE_UNIT_TESTS)
  add_subdirectory(googletest/googletest)
endif()


if(SMF_ENABLE_BENCHMARK_TESTS)
  SMF_SET_OPTION(BENCHMARK_ENABLE_GTEST_TESTS OFF)
  SMF_SET_OPTION(BENCHMARK_ENABLE_TESTING OFF)
  add_subdirectory(google_benchmark)
endif()
