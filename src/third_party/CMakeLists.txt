# these are a bit hacky, and should eventually be replaced by proper cmake find
# package modules and/or create proper targets. at this point the build system
# is simple enough that its tractable to deal with these few special cases.
SMF_SET_OPTION(FLATBUFFERS_BUILD_TESTS OFF)
SMF_SET_OPTION(FLATBUFFERS_INSTALL OFF)
SMF_SET_OPTION(FLATBUFFERS_BUILD_FLATHASH OFF)
add_subdirectory(flatbuffers)

find_package(fmt 5.2.1 QUIET)
if(NOT fmt_FOUND)
  message(STATUS "Could not find fmt, will build it")
  add_subdirectory(fmt)
elseif(fmt_VERSION VERSION_GREATER_EQUAL 5.2.1)
  message(WARNING "Could NOT find fmt: "
    "Found unsuitable version \"${fmt_VERSION}\", "
    "but required is at most \"5.2.1\" (found ${fmt_INCLUDE_DIR}). "
    "Will build it")
  add_subdirectory(fmt)
endif()

set(SEASTAR_USER_CFLAGS
  -Wno-stringop-overflow
  -Wno-array-bounds
  -Wno-stringop-truncation
  -Wno-format-overflow
  CACHE STRING "User cflags for seastar deps")
SMF_SET_OPTION(SEASTAR_ENABLE_TESTS OFF)

# without this macro, seastar cmake won't detect the correct version of fmt (see
# above). this technique is taken from Ceph (see: <ceph>/src/CMakeLists.txt),
# but it isn't clear how this works. after adopting the 'cmake-cooking.sh'
# method, this will probably vanish ;)
macro(find_package name)
  if(NOT TARGET ${name})
    _find_package(${ARGV})
  endif()
endmacro ()

add_subdirectory(seastar)


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
