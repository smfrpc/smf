set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third_party)

# these are a bit hacky, and should eventually be replaced by proper cmake find
# package modules and/or create proper targets. at this point the build system
# is simple enough that its tractable to deal with these few special cases.
include_directories(third_party/flatbuffers/include)
include_directories(third_party/zstd/lib)
include_directories(third_party/fastrange)
include_directories(SYSTEM third_party/flat_hash_map)

find_package(PkgConfig REQUIRED)
if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${SEASTAR_DIR}/build/debug")
else()
  set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${SEASTAR_DIR}/build/release")
endif()
pkg_search_module(SEASTAR REQUIRED seastar)

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

include(${PROJECT_SOURCE_DIR}/CMake/smf.cmake)

add_subdirectory(platform)
add_subdirectory(utils)
add_subdirectory(histogram)
# promote smf_gen to top-level dir?
add_subdirectory(rpc)

_smf_generate_cpp(rpc rpc_fbs rpc/rpc.fbs)

set(rpc_srcs
  rpc/rpc_handle_router.cc
  rpc/rpc_envelope.cc
  rpc/rpc_server.cc
  rpc/rpc_recv_context.cc
  rpc/rpc_client.cc
  rpc/rpc_client_stats.cc
  rpc/rpc_recv_context.cc
  rpc/rpc_connection_limits.cc
  rpc/rpc_letter.cc
  rpc/filters/zstd_filter.cc
  rpc/filters/lz4_filter.cc
  ${rpc_fbs_OUTPUTS})

set(util_srcs
  utils/random.cc
  utils/checks/cpu.cc
  utils/checks/memory.cc
  utils/checks/disk.cc
  utils/human_bytes.cc
  utils/compression.cc)

set(histogram_srcs
  histogram/histogram.cc
  histogram/histogram_seastar_utils.cc)

add_library(smf STATIC
  ${rpc_srcs}
  ${util_srcs}
  ${histogram_srcs})

target_compile_options(smf
  PUBLIC ${SEASTAR_CFLAGS})

target_include_directories(smf
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC third_party/flatbuffers/include
  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/xxHash
  PUBLIC ${SEASTAR_INCLUDE_DIRS})

target_compile_definitions(smf
  PUBLIC -DXXH_PRIVATE_API)

target_link_libraries(smf
  ${SEASTAR_LDFLAGS}
  libzstd_static
  hdr_histogram_static
  lz4)

install(TARGETS smf DESTINATION lib)

if(SMF_BUILD_PROGRAMS)
  add_subdirectory(demo_apps)
endif()

if(SMF_ENABLE_BENCHMARK_TESTS)
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable Google benchmark testing")
  set(
    CMAKE_BUILD_TYPE Release
    PATH third_party/google_benchmark)
  add_subdirectory(third_party/google_benchmark)
  add_subdirectory(benchmarks)
endif(SMF_ENABLE_BENCHMARK_TESTS)

if(SMF_ENABLE_INTEGRATION_TESTS)
  add_subdirectory(integration_tests)
endif(SMF_ENABLE_INTEGRATION_TESTS)
