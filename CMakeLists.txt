cmake_minimum_required(VERSION 3.5) # 3.5 required by seastar
list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (Cooking OPTIONAL)
project(SMF VERSION "0.1.0" LANGUAGES CXX C)
# https://cmake.org/cmake/help/v3.4/policy/CMP0065.html
cmake_policy(SET CMP0065 OLD)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -v")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()

option(SMF_INSTALL "Enable installation of smf" ON)
option(SMF_ENABLE_TESTS "Useful for disabling all tests" ON)
option(SMF_ENABLE_INTEGRATION_TESTS "control if integrations are bulit and ran" ON)
option(SMF_ENABLE_UNIT_TESTS "control if unit tests are bulit and ran" ON)
option(SMF_ENABLE_BENCHMARK_TESTS "control if benchmarks are bulit and ran" OFF)
option(SMF_ENABLE_CMAKE_PROJECT_FLAGS "control cmake_cxx_flags_*" OFF)
option(SMF_BUILD_PROGRAMS "Build smf programs and demos" ON)

if(NOT SMF_ENABLE_TESTS)
  set(SMF_ENABLE_INTEGRATION_TESTS  OFF)
  set(SMF_ENABLE_UNIT_TESTS  OFF)
  set(SMF_ENABLE_BENCHMARK_TESTS  OFF)
endif()

include(ccache)
include(doxygen_gen)
include(packaging)
include(set_option)
include(tests)
include(smfc_generator)

# must come before installation
# Because of a cmake bugs. One can only include
# targets defined in the same file. wut!...
# So here life becomes a french farce and we have to inline a bunch of code
# https://cmake.org/Bug/view.php?id=14444
find_package (Flatbuffers REQUIRED)
find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(Boost COMPONENTS
  system
  filesystem
  REQUIRED)
find_package (Threads)

set(SMFC_SOURCES
  src/smfc/codegen.cc
  src/smfc/cpp_generator.cc
  src/smfc/go_generator.cc
  src/smfc/main.cc)

set(SMFC_LIBS
  flatbuffers::flatbuffers
  Boost::system
  Boost::filesystem
  gflags
  glog::glog
  ${CMAKE_THREAD_LIBS_INIT}
  )

add_executable(smfc ${SMFC_SOURCES})
target_link_libraries(smfc PRIVATE ${SMFC_LIBS})
target_include_directories(smfc PRIVATE src/smfc)


#find_package(Flatbuffers REQUIRED)
find_package(Zstd REQUIRED)
find_package(Hdrhistogram REQUIRED)
find_package(xxHash REQUIRED)
find_package(Seastar 1.0 REQUIRED)

smfc_gen(
  CPP
  TARGET_NAME raw_rpc_gen
  OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/src/include/smf
  SOURCES ${PROJECT_SOURCE_DIR}/src/core/rpc.fbs)


file(GLOB smf_includes "${CMAKE_CURRENT_SOURCE_DIR}/src/include/*.h")
file(GLOB smf_srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cc")
add_library(smf STATIC
  ${raw_rpc_gen}
  ${smf_srcs}
  ${smf_includes}
  )
set_target_properties(smf PROPERTIES VERSION ${PROJECT_VERSION})
target_include_directories(smf PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
    $<INSTALL_INTERFACE:include>  # <prefix>/include
)
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

target_compile_definitions(smf PUBLIC -DXXH_PRIVATE_API)
target_link_libraries(smf
  PUBLIC
  Seastar::seastar
  flatbuffers::flatbuffers
  xxHash::xxhash
  Zstd::zstd
  Hdrhistogram::hdr_histogram)

# we need the compiler to generate the rpc header
add_dependencies(smf smfc)


if(SMF_ENABLE_BENCHMARK_TESTS)
  add_subdirectory(src/benchmarks)
endif()
if(SMF_ENABLE_INTEGRATION_TESTS)
  add_subdirectory(src/integration_tests)
endif()
if(SMF_ENABLE_UNIT_TESTS)
  add_subdirectory(src/tests)
endif()
if(SMF_BUILD_PROGRAMS)
  add_subdirectory(demo_apps)
endif()

# configuration for downstream projects
if(SMF_INSTALL)
  include (GNUInstallDirs)
  include (CMakePackageConfigHelpers)
  set(install_cmakedir ${CMAKE_INSTALL_LIBDIR}/cmake/smf)
  install (
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install (
    TARGETS
      smfc
    EXPORT smf-export
    DESTINATION ${CMAKE_INSTALL_BINDIR})
  install (
    TARGETS
      smf
    EXPORT smf-export
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install (
    EXPORT smf-export
    FILE smfTargets.cmake
    NAMESPACE smf::
    DESTINATION ${install_cmakedir})
  configure_package_config_file (
    ${CMAKE_CURRENT_LIST_DIR}/cmake/smf-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/smf-config.cmake
    INSTALL_DESTINATION ${install_cmakedir})
  write_basic_package_version_file (
    ${CMAKE_CURRENT_BINARY_DIR}/smf-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion)
  install (
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/smf-config.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindBoost.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/smfc_generator.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindZstd.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindxxHash.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindHdrhistogram.cmake
    DESTINATION ${install_cmakedir})
  export (
    EXPORT smf-export
    FILE ${CMAKE_CURRENT_BINARY_DIR}/smfTargets.cmake
    NAMESPACE smf::)
  export (PACKAGE smf)
endif()
