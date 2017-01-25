cmake_minimum_required(VERSION 3.0)
set(SMF_VERSION "0.0.1")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
#set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -v" )
include(CMake/includes.cmake)
include(CMake/gen_flatbuffers.cmake)
include(CMake/Seastar.cmake)
include(CMake/ccache.cmake)
include(CMake/base_flags.cmake)
include(CMake/join_function.cmake)
include(CMake/rpc_lib_sources.cmake)
include(CMake/xxhash.cmake)
message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
enable_testing()
set(CXX_FLAG_LIST
  ${BASE_FLAGS}
  ${SEASTAR_CFLAGS}
  )
join("${CXX_FLAG_LIST}" " " CMAKE_CXX_FLAGS)
add_library(smftracing STATIC
  ${PROJECT_SOURCE_DIR}/src/histogram.cc
  ${PROJECT_SOURCE_DIR}/src/histogram_seastar_utils.cc
  )
target_link_libraries(smftracing
  hdr_histogram_static
  ${SEASTAR_LDFLAGS}
  )
add_library(smfrpc STATIC ${RPC_LIB_SOURCES})
target_link_libraries(smfrpc
  ${SEASTAR_LDFLAGS}
  smftracing
  xxhash_static
  zstd.a
  )
add_dependencies(smfrpc rpc_serialization)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/rpc/smf_gen)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/raft)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/chain_replication)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/filesystem)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/smfb)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/test)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/integration_tests)
