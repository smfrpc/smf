cmake_minimum_required(VERSION 3.9)
project(SMF VERSION "0.1.0" LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -v")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()

option(SMF_BUILD_PROGRAMS "Build smf programs and demos" ON)
option(SMF_ENABLE_INTEGRATION_TESTS "control if integrations are bulit and ran" ON)
option(SMF_ENABLE_UNIT_TESTS "control if unit tests are bulit and ran" ON)
option(SMF_ENABLE_BENCHMARK_TESTS "control if benchmarks are bulit and ran" OFF)
option(SMF_INSTALL "Enable installation of smf" ON)
option(SMF_ENABLE_TESTS "Useful for disabling all tests" ON)

# fetch and build dependencies. each dependency is built in isolation allowing
# tight control over the build configuration, and avoiding any inadvertant cmake
# cache pollution. the dependencies are built at configure-time and then
# imported as normal cmake dependencies, which will easily enable future
# iterations to selectively satisfy dependencies from system packages.
configure_file(CMakeLists.txt.in ${CMAKE_CURRENT_BINARY_DIR}/deps-build/CMakeLists.txt @ONLY)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/deps-build)
if(result)
  message(FATAL_ERROR "CMake step for deps-build failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/deps-build)
if(result)
  message(FATAL_ERROR "Build step for deps-build failed: ${result}")
endif()

# wire up dependency search paths. most of the dependencies land in
# `deps-install` directory. the exception is seastar which we are currently
# depending on to build some of its own depenencies. TODO: once we start
# building seastar's dependencies too, we can install seastar and this will be
# an even cleaner path configuration.
foreach(dep_dir
    "seastar_ext-prefix/src/seastar_ext/build/release"
    "seastar_ext-prefix/src/seastar_ext/build/release/_cooking/installed"
    "deps-install")
  list(APPEND CMAKE_PREFIX_PATH
    "${CMAKE_CURRENT_BINARY_DIR}/deps-build/${dep_dir}")
endforeach()

# seastar includes several custom cmake modules
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_BINARY_DIR}/deps-build/seastar_ext-prefix/src/seastar_ext/cmake")

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
include(packaging)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindBoost.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/smfc_generator.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindZstd.cmake
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
