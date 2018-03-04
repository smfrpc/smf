include(${PROJECT_SOURCE_DIR}/CMake/smf.cmake)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -ggdb")

add_subdirectory(third_party)

if(SMF_BUILD_COMPILER)
  add_subdirectory(smfc)
endif()

if(SMF_BUILD_LIBSMF)
  add_subdirectory(core)
endif()


if(SMF_ENABLE_BENCHMARK_TESTS)
  add_subdirectory(benchmarks)
endif(SMF_ENABLE_BENCHMARK_TESTS)

if(SMF_ENABLE_INTEGRATION_TESTS)
  add_subdirectory(integration_tests)
endif(SMF_ENABLE_INTEGRATION_TESTS)
