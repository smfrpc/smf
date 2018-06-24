find_package(GFlags)
find_package(Glog)
find_package(Boost COMPONENTS
  system
  filesystem
  REQUIRED)

set(SMFC_SOURCES
  codegen.cc
  cpp_generator.cc
  go_generator.cc
  java_generator.cc
  main.cc)

set(SMFC_LIBS
  flatbuffers
  boost_system
  boost_filesystem
  gflags
  glog
  pthread
  )

add_executable (smfc ${SMFC_SOURCES})
target_link_libraries (smfc ${SMFC_LIBS})
target_include_directories(smfc
  PUBLIC ${PROJECT_SOURCE_DIR}/src/third_party/flatbuffers/include)
install(TARGETS smfc RUNTIME DESTINATION bin)
target_compile_definitions(smfc PRIVATE -DXXH_PRIVATE_API)
