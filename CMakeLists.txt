cmake_minimum_required(VERSION 3.0)
set(SMURF_VERSION "0.0.1")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(FLATBUFFERS_FLATC_EXECUTABLE ${PROJECT_SOURCE_DIR}/src/third_party/bin/flatc)
set(FLATBUFFERS_FLATC_SCHEMA_EXTRA_ARGS --gen-name-strings)
INCLUDE(CMake/BuildFlatBuffers.cmake)
INCLUDE(CMake/Seastar.cmake)
INCLUDE(CMake/ccache.cmake)
INCLUDE(CMake/base_flags.cmake)
INCLUDE(CMake/join_function.cmake)
INCLUDE(CMake/rpc_lib_sources.cmake)
INCLUDE(CMake/smf_gen_sources.cmake)

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
  "${PROJECT_SOURCE_DIR}/src/flatbuffers"
  )
# Compile the Types for the RPC tests - kinda recursive - sigh
build_flatbuffers(
  #flatbuffers_schemas
  "${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/demo_service.fbs"
  #schema_include_dirs
  "${PROJECT_SOURCE_DIR}/src/rpc/smf_gen"
  #custom_target_name
  smf_gen_demo_types
  #additional_dependencies
  ""
  #generated_includes_dir
  "${PROJECT_SOURCE_DIR}/src/rpc/smf_gen"
  #binary_schemas_dir
  "${PROJECT_SOURCE_DIR}/src/rpc/smf_gen"
  #copy_text_schemas_dir
  "${PROJECT_SOURCE_DIR}/src/rpc/smf_gen"
  )

set(CXX_FLAG_LIST
  ${SEASTAR_CFLAGS}
  ${BASE_FLAGS}
  )
#set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -v" )
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
  )

# generator
add_executable (smf_gen ${SMF_GEN_SOURCES})
target_link_libraries (smf_gen ${BASE_FLAG} ${SMF_GEN_LIBS})
# template server
add_executable (rpc_template_server ${PROJECT_SOURCE_DIR}/src/rpc/templates/server.cc)
target_link_libraries (rpc_template_server smfrpc)
# template client
add_executable (rpc_template_client ${PROJECT_SOURCE_DIR}/src/rpc/templates/client.cc)
target_link_libraries (rpc_template_client smfrpc)



# run all the tests
add_subdirectory (${PROJECT_SOURCE_DIR}/src/test)
