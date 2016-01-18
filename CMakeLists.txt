cmake_minimum_required (VERSION 2.6)
set(SMURF_VERSION "0.0.1")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

enable_testing ()
# TODO(agallego) -
# Do what we do for concord - install in tmp/toor/* directory
# so you only include one path


# Don't forget to add them to the run_test.sh
link_directories (
  ${PROTOBUF_LIB_PATH}
  ${PROJECT_SOURCE_DIR}/meta/tmp/folly/folly/.libs
  ${PROJECT_SOURCE_DIR}/meta/tmp/nvml/src/nondebug
  /usr/local/lib # must be last if locals aren't used
  )

include_directories (
  SYSTEM
  ${PROJECT_SOURCE_DIR}/meta/tmp/folly/folly
  ${PROJECT_SOURCE_DIR}/meta/tmp/nvml/src/include
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
  "-ggdb"
  )

set (CXX_FLAG_LIST
  ${BASE_FLAGS}
  "-std=c++1y"
  "-ftemplate-depth-256"
  "-Woverloaded-virtual"
  "-Wsign-promo"
  "-Wreorder"
  )

# set(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
# set(GCC_COVERAGE_LINK_FLAGS    "-lgcov")
# set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -v" )

join ("${CXX_FLAG_LIST}" " " CMAKE_CXX_FLAGS)
join ("${BASE_FLAGS}" " " CMAKE_C_FLAGS)
set (CMAKE_CXX_COMPILER g++-5)
set (CMAKE_C_COMPILER gcc-5)

set (LIBFOLLY
  :libfolly.so.57.0.0
  :libboost_filesystem.so.1.55.0
  :libboost_system.so.1.55.0
  :libboost_regex.so.1.55.0
  :libpthread.so.0
  :liblzma.so.5
  :libz.so.1
  :libsnappy.so.1
  :liblz4.so.1
  :libjemalloc.so.1
  :libevent-2.0.so.5
  :libdouble-conversion.so.1
  :libssl.so.1.0.0
  :libgflags.so.2
  :libglog.so.0
  :libstdc++.so.6   #g++'s runtime
  :libm.so.6        #math for libc
  :libc.so.6        #c lib
  :libgcc_s.so.1    #gcc's runtime
  :libicuuc.so.52   #unicode
  :libicui18n.so.52 #int'lization
  :libdl.so.2       #dyn loading
  :libcrypto.so.1.0.0
  :libunwind.so.8   #stack unwind
  :libicudata.so.52 #not sure
  )

add_executable (gbly
  ${PROJECT_SOURCE_DIR}/src/smf/main.cc
  )
target_link_libraries (gbly ${LIBFOLLY} ${LIBGRPC})

#add_subdirectory (test)
