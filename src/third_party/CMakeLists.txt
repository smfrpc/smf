# these are a bit hacky, and should eventually be replaced by proper cmake find
# package modules and/or create proper targets. at this point the build system
# is simple enough that its tractable to deal with these few special cases.
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL
  "Disable building flatbuffers tests")
set(FLATBUFFERS_INSTALL OFF CACHE BOOL
  "Disable installing flatbuffers")
set(FLATBUFFERS_BUILD_FLATHASH OFF CACHE BOOL
  "Disable building flathash")
add_subdirectory(flatbuffers)


set(SEASTAR_ENABLE_DPDK ON CACHE BOOL "enable seastar DPDK")
add_subdirectory(seastar)


set(HDR_HISTOGRAM_BUILD_SHARED OFF CACHE BOOL
  "Disable building hdr_histogram shared library")
set(HDR_HISTOGRAM_BUILD_PROGRAMS OFF CACHE BOOL
  "Disable building hdr_histogram tests and examples")
add_subdirectory(HdrHistogram_c)

set(ZSTD_MULTITHREAD_SUPPORT OFF CACHE BOOL
  "No need for multi threads")
set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL
  "No need to support pre 1.0")
set(ZSTD_BUILD_STATIC ON CACHE BOOL
  "Enable building zstd static library")
set(ZSTD_BUILD_SHARED OFF CACHE BOOL
  "Disable building zstd shared library")
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL
  "Disable building zstd programs")
add_subdirectory(zstd/build/cmake)


if(SMF_ENABLE_UNIT_TESTS)
  add_subdirectory(googletest/googletest)
endif()


if(SMF_ENABLE_BENCHMARK_TESTS)
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable Google benchmark testing")
  add_subdirectory(google_benchmark)
endif()
