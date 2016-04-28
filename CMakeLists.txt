cmake_minimum_required (VERSION 2.6)
set(SMURF_VERSION "0.0.1")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

enable_testing ()
# TODO(agallego) -
# Do what we do for concord - install in tmp/toor/* directory
# so you only include one path


# Don't forget to add them to the run_test.sh
link_directories (
  ${PROJECT_SOURCE_DIR}/src/third_party/lib
  ${PROJECT_SOURCE_DIR}/src/third_party/lib64
  /usr/local/lib # must be last if locals aren't used
  )

include_directories (
  SYSTEM
  ${PROJECT_SOURCE_DIR}/src/third_party/include
  ${PROJECT_SOURCE_DIR}/src/third_party/include/seastar
  ${PROJECT_SOURCE_DIR}/src/third_party/include/dpdk
  )
include_directories (
  ${PROJECT_SOURCE_DIR}/src
  )

function (join VALUES SEP OUTPUT)
  string (REPLACE ";" "${SEP}" _TMP_STR "${VALUES}")
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction ()


# TODO(agallego): So these flags work on my desktop.
# specially the avx cpu flag, etc. THese are proc specific/architect specific
# So def not gna work for every computer.
# Refactor, but not sure why cmake can't read the pkg-config thing :( ugh
#
set(SEASTAR_CFLAGS
  -DHAVE_DPDK
  -DRTE_MACHINE_CPUFLAG_SSE
  -DRTE_MACHINE_CPUFLAG_SSE2
  -DRTE_MACHINE_CPUFLAG_SSE3
  -DRTE_MACHINE_CPUFLAG_SSSE3
  -DRTE_MACHINE_CPUFLAG_SSE4_1
  -DRTE_MACHINE_CPUFLAG_SSE4_2
  -DRTE_MACHINE_CPUFLAG_AES
  -DRTE_MACHINE_CPUFLAG_PCLMULQDQ
  -DRTE_MACHINE_CPUFLAG_AVX
  -DRTE_COMPILE_TIME_CPUFLAGS=RTE_CPUFLAG_SSE,RTE_CPUFLAG_SSE2,RTE_CPUFLAG_SSE3,RTE_CPUFLAG_SSSE3,RTE_CPUFLAG_SSE4_1,RTE_CPUFLAG_SSE4_2,RTE_CPUFLAG_AES,RTE_CPUFLAG_PCLMULQDQ,RTE_CPUFLAG_AVX
  -DHAVE_XEN
  -DHAVE_HWLOC
  -DHAVE_NUMA
  )

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

# set(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
# set(GCC_COVERAGE_LINK_FLAGS    "-lgcov")
# set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -v" )

join ("${CXX_FLAG_LIST}" " " CMAKE_CXX_FLAGS)
join ("${BASE_FLAGS}" " " CMAKE_C_FLAGS)
set (CMAKE_CXX_COMPILER g++-5)
set (CMAKE_C_COMPILER gcc-5)

set (LIBSEASTAR
 -Wl,--whole-archive,-lseastar,--no-whole-archive
  pthread
  aio
  boost_program_options
  boost_system
  boost_filesystem
  stdc++
  m
  boost_thread
  cryptopp
  rt
  gnutls
  gnutlsxx
  xenstore
  rte_pmd_vmxnet3_uio
  rte_pmd_i40e
  rte_pmd_ixgbe
  rte_pmd_e1000
  rte_pmd_ring
  rte_hash
  rte_kvargs
  rte_mbuf
  ethdev
  rte_eal
  rte_malloc
  rte_mempool
  rte_ring
  rte_cmdline
  rte_cfgfile
  rt
  m
  dl
  hwloc
  numa
  pciaccess
  xml2
  z
  )

add_executable (smf_server
  ${PROJECT_SOURCE_DIR}/src/smf/server.cc
  )
target_link_libraries (smf_server ${LIBSEASTAR})

add_executable (smf_client
  ${PROJECT_SOURCE_DIR}/src/smf/client.cc
  )
target_link_libraries (smf_client ${LIBSEASTAR})

#add_subdirectory (test)
