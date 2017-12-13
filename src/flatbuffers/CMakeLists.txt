# this is a work-in-progress helper. currently it lets us reference the
# generated service from smf_gen as a normal library dependency, instead of
# using explicit add_dependencies. this needs some work. in particular, in
# dealing with the relationship between smf_gen and flatc.
function(smf_gen_helper target source)
  get_filename_component(output ${source} NAME_WE)
  set(output "${CMAKE_CURRENT_BINARY_DIR}/${output}.smf.fb.h")
  add_custom_command(OUTPUT ${output}
    COMMAND smf_gen
    ARGS --logtostderr --filename ${CMAKE_CURRENT_SOURCE_DIR}/${source} --output_path=${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS smf_gen
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source}
    COMMENT "Running smf_gen on ${source}")
  add_custom_target(${target}_generated DEPENDS ${output})
  add_library(${target} INTERFACE)
  target_include_directories(${target} INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
  add_dependencies(${target}
    ${target}_generated
    rpc_serialization)
endfunction(smf_gen_helper)

set(FLATBUFFERS_FLATC_SCHEMA_EXTRA_ARGS
  --gen-name-strings
  --gen-object-api
  --cpp
  --json
  --reflect-names
  --defaults-json
  --cpp-str-type 'seastar::sstring'
)

# TODO: fine-grained code generation/dependencies
set(FLATBUFFERS_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/rpc.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/raft.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/chain_replication.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/wal.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/demo_service.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/timestamp.fbs)

build_flatbuffers(
  "${FLATBUFFERS_FILES}"
  ""
  rpc_serialization
  ""
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ""
 )

smf_gen_helper(demo_service demo_service.fbs)
smf_gen_helper(cr_service chain_replication.fbs)
