cmake_minimum_required (VERSION 3.0)
set(SMURF_VERSION "0.0.1")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

find_package(PkgConfig REQUIRED)
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${PROJECT_SOURCE_DIR}/meta/tmp/seastar/build/release")
pkg_search_module(SEASTAR REQUIRED seastar)
MESSAGE(STATUS "SEASTAR SEASTAR_FOUND: " ${SEASTAR_FOUND})
MESSAGE(STATUS "SEASTAR SEASTAR_LIBRARIES: " ${SEASTAR_LIBRARIES})
MESSAGE(STATUS "SEASTAR SEASTAR_LIBRARY_DIRS: " ${SEASTAR_LIBRARY_DIRS})
MESSAGE(STATUS "SEASTAR SEASTAR_LDFLAGS: " ${SEASTAR_LDFLAGS})
MESSAGE(STATUS "SEASTAR SEASTAR_LDFLAGS_OTHER: " ${SEASTAR_LDFLAGS_OTHER})
MESSAGE(STATUS "SEASTAR SEASTAR_INCLUDE_DIRS: " ${SEASTAR_INCLUDE_DIRS})
MESSAGE(STATUS "SEASTAR SEASTAR_CFLAGS: " ${SEASTAR_CFLAGS})
MESSAGE(STATUS "SEASTAR CFLAGS_OTHER: " ${SEASTAR_CFLAGS_OTHER})

enable_testing ()
link_directories (
  ${SEASTAR_LIBRARY_DIRS}
  ${PROJECT_SOURCE_DIR}/src/third_party/lib
  ${PROJECT_SOURCE_DIR}/src/third_party/lib64
  /usr/local/lib # must be last if locals aren't used
  )
include_directories (
  SYSTEM
  ${SEASTAR_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/src/third_party/include
  )
include_directories (
  ${PROJECT_SOURCE_DIR}/src
  )
function (join VALUES SEP OUTPUT)
  string (REPLACE ";" "${SEP}" _TMP_STR "${VALUES}")
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction ()



set(BASE_FLAGS
  "-fPIC"
  "-fuse-ld=gold"
  "-Wall"
  "-Werror"
  "-Wextra"
  "-Wformat"
  "-Wmissing-braces"
  "-Wparentheses"
  "-Wpointer-arith"
  "-Wformat-security"
  "-Wunused"
  "-Wno-unused-parameter"
  "-Wcast-align"
  "-Wno-missing-field-initializers"
  "-Wno-ignored-qualifiers"
  "-O2"
  )
set (CXX_FLAG_LIST
  ${SEASTAR_CFLAGS}
  ${BASE_FLAGS}
  )


set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -v" )
join ("${CXX_FLAG_LIST}" " " CMAKE_CXX_FLAGS)
MESSAGE(STATUS "CXX FLAGS: " ${CXX_FLAG_LIST})
add_executable (smf_server ${PROJECT_SOURCE_DIR}/src/smf/server.cc)
target_link_libraries (smf_server ${SEASTAR_LDFLAGS})
add_executable (smf_client ${PROJECT_SOURCE_DIR}/src/smf/client.cc)
target_link_libraries (smf_client ${SEASTAR_LDFLAGS})

#add_subdirectory (test)
