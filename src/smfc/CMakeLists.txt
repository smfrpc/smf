find_package (Flatbuffers REQUIRED)
find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(Boost COMPONENTS
  system
  filesystem
  REQUIRED)
find_package (Threads)

set(SMFC_SOURCES
  codegen.cc
  cpp_generator.cc
  go_generator.cc
  main.cc)

set(SMFC_LIBS
  flatbuffers::flatbuffers
  Boost::system
  Boost::filesystem
  gflags
  glog::glog
  ${CMAKE_THREAD_LIBS_INIT}
  )

add_executable (smfc ${SMFC_SOURCES})
target_link_libraries (smfc PRIVATE ${SMFC_LIBS})
install(TARGETS smfc RUNTIME DESTINATION bin)
