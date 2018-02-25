add_subdirectory(third_party/HdrHistogram_c)
add_subdirectory(third_party/fmt)

add_subdirectory(third_party/flatbuffers)
include_directories(third_party/flatbuffers/include)

set(ZSTD_BUILD_SHARED ON CACHE BOOL
  "Enable building zstd static library")
set(ZSTD_BUILD_SHARED OFF CACHE BOOL
  "Disable building zstd shared library")
add_subdirectory(third_party/zstd/build/cmake)
include_directories(third_party/zstd/lib)

include_directories(third_party/fastrange)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${SEASTAR_DIR}/build/debug")
else()
  set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${SEASTAR_DIR}/build/release")
endif()
pkg_search_module(SEASTAR REQUIRED seastar)

add_library(seastar INTERFACE)
target_link_libraries(seastar INTERFACE ${SEASTAR_LDFLAGS})
target_compile_options(seastar
  INTERFACE ${SEASTAR_CFLAGS})
target_include_directories(seastar
  INTERFACE ${SEASTAR_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto -Wall -Werror -Wextra -Wformat")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmissing-braces -Wparentheses -Wpointer-arith")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat-security -Wunused -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcast-align -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdelete-non-virtual-dtor -Wno-ignored-qualifiers")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.1)
    add_definitions(-DSMF_GCC_CONCEPTS=1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts")
    if(CMAKE_BUILD_TYPE MATCHES Debug)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lasan -lubsan")
    endif()
  endif()
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -ggdb")

include(${CMAKE_SOURCE_DIR}/CMake/smf.cmake)

add_subdirectory(platform)
add_subdirectory(utils)
add_subdirectory(hashing)
add_subdirectory(histogram)
add_subdirectory(rpc)
add_subdirectory(demo_apps)

if(ENABLE_UNIT_TESTS)
  add_subdirectory(googletest/googlemock)
endif()

if(ENABLE_BENCHMARK_TESTS)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable Google benchmark testing")
  add_subdirectory(third_party/google_benchmark)
  add_subdirectory(benchmarks)
endif(ENABLE_BENCHMARK_TESTS)

if(ENABLE_INTEGRATION_TESTS)
  add_subdirectory(integration_tests)
endif(ENABLE_INTEGRATION_TESTS)
