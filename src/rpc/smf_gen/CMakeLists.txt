set(SMF_GEN_SOURCES
  ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/cpp_generator.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/idl.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/main.cc
  )
set(SMF_GEN_LIBS
  flatbuffers.a
  glog
  gflags
  pthread
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
  ""
  )

# generator
add_executable (smf_gen ${SMF_GEN_SOURCES})
target_link_libraries (smf_gen ${BASE_FLAG} ${SMF_GEN_LIBS})
add_dependencies(smf_gen smf_gen_demo_types)
install(TARGETS smf_gen RUNTIME DESTINATION bin)
