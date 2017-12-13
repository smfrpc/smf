cmake_minimum_required(VERSION 3.3)
project(SMF VERSION "0.0.1" LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/")

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

option(ENABLE_INTEGRATION_TESTS "control if integrations are bulit and ran" ON)
message(STATUS "ENABLE_INTEGRATION_TESTS=${ENABLE_INTEGRATION_TESTS}")

option(ENABLE_UNIT_TESTS "control if tests are bulit and ran" ON)
message(STATUS "ENABLE_UNIT_TESTS=${ENABLE_UNIT_TESTS}")

include_directories(
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/src)

include(CMake/doxygen.cmake)
include(CMake/ccache.cmake)
include(CMake/packaging.cmake)

add_subdirectory(src)
