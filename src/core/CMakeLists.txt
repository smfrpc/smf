find_package(LZ4)
find_package(Boost COMPONENTS
  system
  program_options
  REQUIRED)
find_package(PkgConfig REQUIRED)
smfc_gen(
  CPP
  TARGET_NAME raw_rpc_gen
  OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/src/smf
  SOURCES ${PROJECT_SOURCE_DIR}/src/core/rpc.fbs)

# seastar dep

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


add_library(smf STATIC
  compression.cc
  histogram.cc
  histogram_seastar_utils.cc
  random.cc
  rpc_client.cc
  rpc_connection_limits.cc
  rpc_envelope.cc
  rpc_handle_router.cc
  rpc_letter.cc
  rpc_recv_context.cc
  rpc_server.cc
  lz4_filter.cc
  zstd_filter.cc
  ${raw_rpc_gen})

target_compile_options(smf
  PUBLIC ${SEASTAR_CFLAGS})

target_include_directories(smf
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  )
target_include_directories(smf
  SYSTEM

  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/HdrHistogram_c/src
  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/flatbuffers/include
  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/flat_hash_map
  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/zstd/lib
  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/fastrange
  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/xxHash
  PUBLIC ${SEASTAR_INCLUDE_DIRS})

target_compile_definitions(smf
  PUBLIC -DXXH_PRIVATE_API)

target_link_libraries(smf
  ${SEASTAR_LDFLAGS}
  libzstd_static
  hdr_histogram_static
  fmt
  lz4)

install(TARGETS smf DESTINATION lib)
install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/src/smf
  DESTINATION include)
