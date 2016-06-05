cmake_minimum_required (VERSION 3.0)
set(SMURF_VERSION "0.0.1")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(FLATBUFFERS_FLATC_EXECUTABLE ${PROJECT_SOURCE_DIR}/src/third_party/bin/flatc)
INCLUDE (CMake/BuildFlatBuffers.cmake)
INCLUDE (CMake/Seastar.cmake)
INCLUDE (CMake/ccache.cmake)
INCLUDE (CMake/base_flags.cmake)
INCLUDE (CMake/join_function.cmake)

message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
enable_testing ()
link_directories (
  ${SEASTAR_LIBRARY_DIRS}
  ${PROJECT_SOURCE_DIR}/src/third_party/lib
  ${PROJECT_SOURCE_DIR}/src/third_party/lib64
  /usr/local/lib # must be last if locals aren't used
  )
include_directories (
  SYSTEM
  ${SEASTAR_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/src/third_party/include
  )
include_directories (
  ${PROJECT_SOURCE_DIR}/src
  )

set (FLATBUFFERS_FILES
  ${PROJECT_SOURCE_DIR}/src/flatbuffers/rpc.fbs
  ${PROJECT_SOURCE_DIR}/src/flatbuffers/raft.fbs
  )

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
  "${PROJECT_SOURCE_DIR}/src/flatbuffers"
  )


set (CXX_FLAG_LIST
  ${SEASTAR_CFLAGS}
  ${BASE_FLAGS}
  )
# set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -v" )
join ("${CXX_FLAG_LIST}" " " CMAKE_CXX_FLAGS)


# Server

add_executable (smf_server ${PROJECT_SOURCE_DIR}/src/smf/server.cc)
add_dependencies(smf_server rpc_serialization)
target_link_libraries (smf_server ${SEASTAR_LDFLAGS})

# Client

add_executable (smf_client ${PROJECT_SOURCE_DIR}/src/smf/client.cc)
add_dependencies(smf_client rpc_serialization)
target_link_libraries (smf_client ${SEASTAR_LDFLAGS})

add_subdirectory (${PROJECT_SOURCE_DIR}/src/test)
