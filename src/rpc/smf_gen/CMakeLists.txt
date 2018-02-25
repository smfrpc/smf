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
  pthread
  fmt
  smf_hashing)

add_executable (smf_gen ${SMF_GEN_SOURCES})
target_link_libraries (smf_gen ${SMF_GEN_LIBS})
target_include_directories(smf_gen
  PUBLIC ${SEASTAR_INCLUDE_DIRS})
install(TARGETS smf_gen RUNTIME DESTINATION bin)
