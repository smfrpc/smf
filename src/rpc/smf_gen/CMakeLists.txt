set(SMF_GEN_SOURCES
  ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/cpp_generator.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/idl.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/main.cc
  )
set(SMF_GEN_LIBS
  flatbuffers.a
  boost_system
  boost_filesystem
  gflags
  glog
  pthread
  )
# generator
add_executable (smf_gen ${SMF_GEN_SOURCES})
target_link_libraries (smf_gen ${BASE_FLAG} ${SMF_GEN_LIBS})
install(TARGETS smf_gen RUNTIME DESTINATION bin)

add_custom_command (
  OUTPUT ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.smf.fb.h
  COMMAND smf_gen --logtostderr --filename ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.fbs
  DEPENDS smf_gen
  DEPENDS ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.fbs
  )
add_executable (generated_demo_service
  ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.smf.fb.h
  ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/dummy.cc
  )
add_dependencies(generated_demo_service smf_gen)
