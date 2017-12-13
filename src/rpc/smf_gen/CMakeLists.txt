set(SMF_GEN_SOURCES
  cpp_generator.cc
  idl.cc
  main.cc)

set(SMF_GEN_LIBS
  flatbuffers
  boost_system
  boost_filesystem
  gflags
  glog
  pthread)

add_executable (smf_gen ${SMF_GEN_SOURCES})
target_link_libraries (smf_gen xxhash ${SMF_GEN_LIBS})
install(TARGETS smf_gen RUNTIME DESTINATION bin)

add_executable(generated_demo_service dummy.cc)
target_link_libraries(generated_demo_service demo_service)
