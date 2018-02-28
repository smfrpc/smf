add_subdirectory(fmt)

set(HDR_HISTOGRAM_BUILD_SHARED OFF CACHE BOOL
  "Disable building hdr_histogram shared library")
set(HDR_HISTOGRAM_BUILD_PROGRAMS OFF CACHE BOOL
  "Disable building hdr_histogram tests and examples")
add_subdirectory(HdrHistogram_c)

set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL
  "Disable building flatbuffers tests")
set(FLATBUFFERS_INSTALL OFF CACHE BOOL
  "Disable installing flatbuffers")
set(FLATBUFFERS_BUILD_FLATHASH OFF CACHE BOOL
  "Disable building flathash")
add_subdirectory(flatbuffers)

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
