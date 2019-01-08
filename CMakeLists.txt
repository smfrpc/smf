cmake_minimum_required(VERSION 3.5) # 3.5 required by seastar
list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (Cooking OPTIONAL)
# https://cmake.org/cmake/help/v3.4/policy/CMP0065.html
project(SMF VERSION "0.1.0" LANGUAGES CXX C)
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

option(SMF_ENABLE_TESTS "Useful for disabling all tests" ON)
option(SMF_ENABLE_INTEGRATION_TESTS "control if integrations are bulit and ran" ON)
option(SMF_ENABLE_UNIT_TESTS "control if unit tests are bulit and ran" ON)
option(SMF_ENABLE_BENCHMARK_TESTS "control if benchmarks are bulit and ran" OFF)
option(SMF_ENABLE_CMAKE_PROJECT_FLAGS "control cmake_cxx_flags_*" OFF)
if(NOT SMF_ENABLE_TESTS)
  set(SMF_ENABLE_INTEGRATION_TESTS  OFF)
  set(SMF_ENABLE_UNIT_TESTS  OFF)
  set(SMF_ENABLE_BENCHMARK_TESTS  OFF)
endif()


option(SMF_BUILD_REGENERATE_RPC_FBS "regenerate the rpc.fbs file" OFF)
option(SMF_BUILD_PROGRAMS "Build smf programs and demos" ON)

include(ccache)
include(doxygen_gen)
include(doxygen_gen)
include(packaging)
include(set_option)
include(tests)

include_directories(
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/src)

add_subdirectory(src)

if(SMF_BUILD_PROGRAMS)
  add_subdirectory(demo_apps)
endif()

