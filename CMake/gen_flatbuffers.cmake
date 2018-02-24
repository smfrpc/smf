set(FLATBUFFERS_FLATC_EXECUTABLE
  ${PROJECT_SOURCE_DIR}/src/third_party/bin/flatc)
set(FLATBUFFERS_FLATC_SCHEMA_EXTRA_ARGS
  --gen-name-strings
  --gen-object-api
  --cpp
  --json
  --reflect-names
  --defaults-json
  -I ${PROJECT_SOURCE_DIR}/src
  --cpp-str-type 'seastar::sstring'
  )
include(${PROJECT_SOURCE_DIR}/CMake/BuildFlatBuffers.cmake)
set(FLATBUFFERS_FILES
  ${PROJECT_SOURCE_DIR}/src/flatbuffers/rpc.fbs
  ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.fbs
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
