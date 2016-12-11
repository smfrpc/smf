cmake_minimum_required(VERSION 3.0)
set(SMF_VERSION "0.0.1")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(FLATBUFFERS_FLATC_EXECUTABLE
  ${PROJECT_SOURCE_DIR}/src/third_party/bin/flatc)
set(FLATBUFFERS_FLATC_SCHEMA_EXTRA_ARGS --gen-name-strings --gen-object-api)
#set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -v" )
include(CMake/BuildFlatBuffers.cmake)
include(CMake/Seastar.cmake)
include(CMake/ccache.cmake)
include(CMake/base_flags.cmake)
include(CMake/join_function.cmake)
include(CMake/rpc_lib_sources.cmake)
include(CMake/xxhash.cmake)

message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
enable_testing()
link_directories(
  ${SEASTAR_LIBRARY_DIRS}
  ${PROJECT_SOURCE_DIR}/src/third_party/lib
  ${PROJECT_SOURCE_DIR}/src/third_party/lib64
  /usr/local/lib # must be last if locals aren't used
  )
include_directories(
  SYSTEM
  ${SEASTAR_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/src/third_party/include
  )
include_directories(
  ${PROJECT_SOURCE_DIR}/src
  )

set(FLATBUFFERS_FILES
  ${PROJECT_SOURCE_DIR}/src/flatbuffers/rpc.fbs
  ${PROJECT_SOURCE_DIR}/src/flatbuffers/raft.fbs
  ${PROJECT_SOURCE_DIR}/src/flatbuffers/chain_replication.fbs
  ${PROJECT_SOURCE_DIR}/src/flatbuffers/wal.fbs
  )

# build the RPC Types
build_flatbuffers(
  #flatbuffers_schemas
  "${FLATBUFFERS_FILES}"
  #schema_include_dirs
  "${PROJECT_SOURCE_DIR}/src/flatbuffers"
  #custom_target_name
  rpc_serialization
  #additional_dependencies
  ""
  #generated_includes_dir
  "${PROJECT_SOURCE_DIR}/src/flatbuffers"
  #binary_schemas_dir
  "${PROJECT_SOURCE_DIR}/src/flatbuffers"
  #copy_text_schemas_dir
  ""
  )
set(CXX_FLAG_LIST
  ${SEASTAR_CFLAGS}
  ${BASE_FLAGS}
  )
join("${CXX_FLAG_LIST}" " " CMAKE_CXX_FLAGS)
add_library(smftracing STATIC
  ${PROJECT_SOURCE_DIR}/src/histogram.cc
  )
target_link_libraries(smftracing hdr_histogram_static)

# smf rpc
add_library(smfrpc STATIC ${RPC_LIB_SOURCES})
target_link_libraries(smfrpc
  ${SEASTAR_LDFLAGS}
  smftracing
  xxhash_static
  zstd.a
  )
add_dependencies(smfrpc rpc_serialization)


# add programs that rely on these common ifaces
add_subdirectory(${PROJECT_SOURCE_DIR}/src/rpc/smf_gen)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/raft)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/chain_replication)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/filesystem)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/smfb)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/test)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/integration_tests)
