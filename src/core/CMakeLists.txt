find_package(LZ4)
find_package(Boost COMPONENTS
  system
  program_options
  REQUIRED)
find_package(PkgConfig REQUIRED)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.1)
    add_definitions(-DSMF_GCC_CONCEPTS=1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts")
    if(CMAKE_BUILD_TYPE MATCHES Debug)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lasan -lubsan")
    endif()
  endif()
endif()

if(SMF_BUILD_REGENERATE_RPC_FBS)
  # This is an exception to what we suggest people to use
  # when using this cmake function
  # We want to generate the rpc.fbs *inside* include/smf
  # so that when other projects link against it, they will
  # be able to build regardless if they are or aren't cmake projects
  #
  smfc_gen(
    CPP
    TARGET_NAME raw_rpc_gen
    OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/src/include/smf
    SOURCES ${PROJECT_SOURCE_DIR}/src/core/rpc.fbs)
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
  # make sure when the dep change, it triggers a rebuild
  ${PROJECT_SOURCE_DIR}/src/include/smf/rpc_generated.h
  )

target_include_directories(smf
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${PROJECT_SOURCE_DIR}/src/include
  )
target_include_directories(smf
  SYSTEM

  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/HdrHistogram_c/src
  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/flatbuffers/include
  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/flat_hash_map
  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/zstd/lib
  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/xxHash
  PUBLIC "${seastar_INCLUDE_DIRS}"
  )
if(CMAKE_BUILD_TYPE MATCHES Release)
  target_compile_options(smf PRIVATE -O3 -g)
else()
  target_compile_options(smf PRIVATE
    -O0
    -ggdb
    -fdiagnostics-color=auto
    -Wall
    -Werror
    -Wextra
    -Wformat
    -Wmissing-braces
    -Wparentheses
    -Wpointer-arith
    -Wformat-security
    -Wunused
    -Wno-unused-parameter
    -Wcast-align
    -Wno-missing-field-initializers
    -Wdelete-non-virtual-dtor
    -Wno-ignored-qualifiers)
endif()

target_compile_definitions(smf
  PUBLIC -DXXH_PRIVATE_API)

target_link_libraries(smf
  seastar
  libzstd_static
  hdr_histogram_static
  fmt
  lz4)

install(TARGETS smf DESTINATION lib)
install(
  FILES ${PROJECT_SOURCE_DIR}/src/core/rpc.fbs
  DESTINATION include/smf)
install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/src/include/smf
  DESTINATION include)
