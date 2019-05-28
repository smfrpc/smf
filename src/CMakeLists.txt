include(GNUInstallDirs)

set(SMFC_SOURCES
  smfc/codegen.cc
  smfc/cpp_generator.cc
  smfc/go_generator.cc
  smfc/python_generator.cc
  smfc/main.cc)

set(SMFC_LIBS
  Flatbuffers::flatbuffers
  Boost::system
  Boost::filesystem
  gflags_static
  glog::glog
  ${CMAKE_THREAD_LIBS_INIT})

add_executable(smfc ${SMFC_SOURCES})
target_link_libraries(smfc PRIVATE ${SMFC_LIBS})
target_include_directories(smfc PRIVATE src/smfc include)
install(TARGETS smfc
  EXPORT smf-export
  DESTINATION ${CMAKE_INSTALL_BINDIR})

smfc_gen(
  CPP
  TARGET_NAME raw_rpc_gen
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/smf
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/core/rpc.fbs)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/include/smf/rpc_generated.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/smf)

file(GLOB smf_srcs "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cc")

add_library(smf STATIC
  ${raw_rpc_gen}
  ${smf_srcs})

add_library(smf::smf ALIAS smf)

set_target_properties(smf PROPERTIES VERSION ${PROJECT_VERSION})

target_include_directories(smf PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_definitions(smf PUBLIC -DXXH_PRIVATE_API)

target_link_libraries(smf
  PUBLIC
  Seastar::seastar
  Flatbuffers::flatbuffers
  Zstd::zstd
  Hdrhistogram::hdr_histogram
  xxHash::xxhash)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_options(smf PRIVATE
    -fdiagnostics-color=auto
    -Wall
    -Werror
    -Wextra
    -Wformat
    -Wmissing-braces
    -Wparentheses
    -Wpointer-arith
    -Wformat-security
    -Wunused
    -Wno-unused-parameter
    -Wcast-align
    -Wno-missing-field-initializers
    -Wdelete-non-virtual-dtor
    -Wno-ignored-qualifiers)
endif()

install(TARGETS smf
  EXPORT smf-export
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(SMF_ENABLE_BENCHMARK_TESTS)
  add_subdirectory(benchmarks)
endif()

if(SMF_ENABLE_INTEGRATION_TESTS)
  add_subdirectory(integration_tests)
endif()

if(SMF_ENABLE_UNIT_TESTS)
  add_subdirectory(tests)
endif()
