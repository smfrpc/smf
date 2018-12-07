# these are a bit hacky, and should eventually be replaced by proper cmake find
# package modules and/or create proper targets. at this point the build system
# is simple enough that its tractable to deal with these few special cases.
SMF_SET_OPTION(FLATBUFFERS_BUILD_TESTS OFF)
SMF_SET_OPTION(FLATBUFFERS_INSTALL OFF)
SMF_SET_OPTION(FLATBUFFERS_BUILD_FLATHASH OFF)
add_subdirectory(flatbuffers)

SMF_SET_OPTION(HDR_HISTOGRAM_BUILD_SHARED OFF)
SMF_SET_OPTION(HDR_HISTOGRAM_BUILD_PROGRAMS OFF)
add_subdirectory(HdrHistogram_c)

SMF_SET_OPTION(ZSTD_MULTITHREAD_SUPPORT OFF)
SMF_SET_OPTION(ZSTD_LEGACY_SUPPORT OFF)
SMF_SET_OPTION(ZSTD_BUILD_STATIC ON)
SMF_SET_OPTION(ZSTD_BUILD_SHARED OFF)
SMF_SET_OPTION(ZSTD_BUILD_PROGRAMS OFF)
add_subdirectory(zstd/build/cmake)


if(SMF_ENABLE_UNIT_TESTS)
  add_subdirectory(googletest/googletest)
endif()


if(SMF_ENABLE_BENCHMARK_TESTS)
  SMF_SET_OPTION(BENCHMARK_ENABLE_GTEST_TESTS OFF)
  SMF_SET_OPTION(BENCHMARK_ENABLE_TESTING OFF)
  add_subdirectory(google_benchmark)
endif()
