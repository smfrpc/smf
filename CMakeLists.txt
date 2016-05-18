cmake_minimum_required (VERSION 3.0)
set(SMURF_VERSION "0.0.1")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(PKG_CONFIG_PATH "${PKG_CONFIG_PATH}:${PROJECT_SOURCE_DIR}/")
pkg_search_module(SEASTAR seastar)

enable_testing ()
  ${PROJECT_SOURCE_DIR}/src/third_party/lib
  ${PROJECT_SOURCE_DIR}/src/third_party/lib64
  /usr/local/lib # must be last if locals aren't used
  )
include_directories (
  SYSTEM
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
  ${SEASAR_CFLAGS}
  "-fPIC"
  "-fuse-ld=gold"
  "-Wall"
  "-Werror"
  "-Wextra"
  "-Wformat"
  "-pedantic"
  "-Wmissing-braces"
  "-Wparentheses"
  "-Wpointer-arith"
  "-Wformat-security"
  "-Wunused"
  "-Wno-unused-parameter"
  "-Wcast-align"
  "-Wmissing-field-initializers"
  "-O2"
  )
set (CXX_FLAG_LIST
  ${BASE_FLAGS}
  "-std=c++1y"
  "-ftemplate-depth-256"
  "-Woverloaded-virtual"
  "-Wsign-promo"
  "-Wreorder"
  "-fvisibility=hidden"
  "-Wno-error=literal-suffix"
  "-Wno-literal-suffix"
  "-Wno-invalid-offsetof"
  )

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -v" )
join ("${CXX_FLAG_LIST}" " " CMAKE_CXX_FLAGS)
join ("${BASE_FLAGS}" " " CMAKE_C_FLAGS)
set (CMAKE_CXX_COMPILER g++-5)
set (CMAKE_C_COMPILER gcc-5)

add_executable (smf_server ${PROJECT_SOURCE_DIR}/src/smf/server.cc)
target_link_libraries (smf_server ${SEASTAR_LIBRARIES})
target_link_libraries (smf_client ${SEASTAR_LIBRARIES})
add_executable (smf_client ${PROJECT_SOURCE_DIR}/src/smf/client.cc)

#add_subdirectory (test)
