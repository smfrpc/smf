cmake_minimum_required(VERSION 3.3)
project(SMF VERSION "0.0.1" LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake/")

# https://cmake.org/cmake/help/v3.4/policy/CMP0065.html
cmake_policy(SET CMP0065 OLD)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

find_package(GFlags)
find_package(Glog)
find_package(LZ4)
find_package(Boost COMPONENTS
  system
  program_options
  REQUIRED)
find_package(Doxygen)

# https://github.com/ceph/ceph/blob/master/CMakeLists.txt
# Use CCACHE_DIR environment variable
option(WITH_CCACHE "Build with ccache.")
if(WITH_CCACHE)
  find_program (CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
    message(STATUS "Building with ccache: ${CCACHE_FOUND}, CCACHE_DIR=$ENV{CCACHE_DIR}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
  else(CCACHE_FOUND)
    message(FATAL_ERROR "Can't find ccache. Is it installed?")
  endif(CCACHE_FOUND)
endif(WITH_CCACHE)

if(DOXYGEN_FOUND)
  configure_file(docs/doxy.in Doxyfile @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)

option(SMF_ENABLE_INTEGRATION_TESTS "control if integrations are bulit and ran" ON)
option(SMF_ENABLE_UNIT_TESTS "control if unit tests are bulit and ran" ON)
option(SMF_ENABLE_BENCHMARK_TESTS "control if benchmarks are bulit and ran" OFF)

include(CMake/packaging.cmake)
include(CMake/tests.cmake)

include_directories(
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/src)

add_subdirectory(src)
