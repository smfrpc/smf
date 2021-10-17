cmake_minimum_required(VERSION 3.7.0)
project(SMF VERSION "0.1.0" LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -v")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SMF_BUILD_PROGRAMS "Build smf programs and demos" ON)
option(SMF_ENABLE_INTEGRATION_TESTS "control if integrations are bulit and ran" ON)
option(SMF_ENABLE_UNIT_TESTS "control if unit tests are bulit and ran" ON)
option(SMF_ENABLE_BENCHMARK_TESTS "control if benchmarks are bulit and ran" OFF)
option(SMF_INSTALL "Enable installation of smf" ON)
option(SMF_ENABLE_TESTS "Useful for disabling all tests" ON)
option(SMF_MANAGE_DEPS "Use smf's reproducible build" OFF)
option(SMF_ENABLE_HWLOC "Enable HWLOC support" OFF)
option(SMF_ENABLE_DPDK "Enable DPDK support" OFF)

if(NOT SMF_DPDK_MACHINE)
  set(SMF_DPDK_MACHINE "native")
endif()

if(SMF_MANAGE_DEPS)
  # don't export() the contents to registry
  set(CMAKE_EXPORT_NO_PACKAGE_REGISTRY ON  CACHE "" INTERNAL FORCE)
  # disable system level registry /usr/local/share/cmake/*
  set(CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY ON  CACHE "" INTERNAL FORCE)
  # disable user package registry ~/.cmake/*
  set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON  CACHE "" INTERNAL FORCE)
  # fetch and build dependencies. each dependency is built in isolation allowing
  # tight control over the build configuration, and avoiding any inadvertant cmake
  # cache pollution. the dependencies are built at configure-time and then
  # imported as normal cmake dependencies, which will easily enable future
  # iterations to selectively satisfy dependencies from system packages.
  set(SMF_DEPS_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/smf-deps-build)
  set(SMF_DEPS_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/smf-deps-install)
  configure_file(CMakeLists.txt.in ${SMF_DEPS_BUILD_DIR}/CMakeLists.txt @ONLY)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${SMF_DEPS_BUILD_DIR})
  if(result)
    message(FATAL_ERROR "CMake step for deps-build failed: ${result}")
  endif()
  execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${SMF_DEPS_BUILD_DIR})
  if(result)
    message(FATAL_ERROR "Build step for deps-build failed: ${result}")
  endif()
  # wire up dependency search paths.
  list(APPEND CMAKE_PREFIX_PATH "${SMF_DEPS_INSTALL_DIR}")
endif()

if(SMF_ENABLE_DPDK)
  find_package(dpdk REQUIRED)
endif()

find_package(Seastar REQUIRED)
find_package(Flatbuffers REQUIRED)
find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(Boost COMPONENTS
  system
  filesystem
  REQUIRED)
find_package (Threads)
find_package(Hdrhistogram REQUIRED)
find_package(xxHash REQUIRED)
find_package(Zstd REQUIRED)

if(NOT SMF_ENABLE_TESTS)
  set(SMF_ENABLE_INTEGRATION_TESTS  OFF)
  set(SMF_ENABLE_UNIT_TESTS  OFF)
  set(SMF_ENABLE_BENCHMARK_TESTS  OFF)
endif()

# put all the libs and binaries in one place
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(smfc_generator)
include(tests)
include(ccache)
include(doxygen_gen)

add_subdirectory(src)

if(SMF_BUILD_PROGRAMS)
  add_subdirectory(demo_apps)
endif()

# configuration for downstream projects
if(SMF_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)
  set(install_cmakedir ${CMAKE_INSTALL_LIBDIR}/cmake/smf)
  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(
    EXPORT smf-export
    FILE smfTargets.cmake
    NAMESPACE smf::
    DESTINATION ${install_cmakedir})
  configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/cmake/smf-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/smf-config.cmake
    INSTALL_DESTINATION ${install_cmakedir})
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/smf-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion)
  install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/smf-config.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/smfc_generator.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindZstd.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Finddpdk.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindxxHash.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindHdrhistogram.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindFlatbuffers.cmake
    DESTINATION ${install_cmakedir})
  export(
    EXPORT smf-export
    FILE ${CMAKE_CURRENT_BINARY_DIR}/smfTargets.cmake
    NAMESPACE smf::)
  export(PACKAGE smf)
endif()
