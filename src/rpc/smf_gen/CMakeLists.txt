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
# generator
add_executable (smf_gen ${SMF_GEN_SOURCES})
target_link_libraries (smf_gen ${BASE_FLAG} ${SMF_GEN_LIBS})
install(TARGETS smf_gen RUNTIME DESTINATION bin)
