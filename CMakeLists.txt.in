cmake_minimum_required(VERSION 3.7.0)
project(build-deps NONE)
cmake_host_system_information(
  RESULT build_concurrency_factor
  QUERY NUMBER_OF_LOGICAL_CORES)
set(CMAKE_EXPORT_NO_PACKAGE_REGISTRY ON  CACHE "" INTERNAL FORCE)
set(CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY ON  CACHE "" INTERNAL FORCE)
set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON  CACHE "" INTERNAL FORCE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(autotools_flags
  CFLAGS=-I@SMF_DEPS_INSTALL_DIR@/include
  CXXFLAGS=-I@SMF_DEPS_INSTALL_DIR@/include
  LDFLAGS=-L@SMF_DEPS_INSTALL_DIR@/lib)
set(PKG_CONFIG_PATH PKG_CONFIG_PATH=@SMF_DEPS_INSTALL_DIR@/lib/pkgconfig)
set(make_command make -j ${build_concurrency_factor})
set(info_dir --infodir=@SMF_DEPS_INSTALL_DIR@/share/info)

include(ExternalProject)

ExternalProject_Add(boost
  URL https://storage.googleapis.com/vectorizedio-public/dependencies/boost_1_75_0.tar.gz
  URL_MD5 38813f6feb40387dfe90160debd71251
  INSTALL_DIR    @SMF_DEPS_INSTALL_DIR@
  PATCH_COMMAND
    ./bootstrap.sh
    --prefix=@SMF_DEPS_INSTALL_DIR@
    --with-libraries=atomic,chrono,date_time,filesystem,program_options,system,test,thread
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND
    ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>
    ./b2
    -j ${build_concurrency_factor}
    --layout=system
    --build-dir=<BINARY_DIR>
    install
    variant=release
    cflags=-fPIC
    cxxflags=-fPIC
    link=static
    threading=multi
    hardcode-dll-paths=true
    dll-path=@SMF_DEPS_INSTALL_DIR@/lib)

ExternalProject_Add(fmt
  URL https://github.com/fmtlib/fmt/archive/9e554999ce02cf86fcdfe74fe740c4fe3f5a56d5.tar.gz
  URL_MD5 e2869a5032a6cc35b4d14008b6125ed9
  INSTALL_DIR    @SMF_DEPS_INSTALL_DIR@
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DFMT_INSTALL=ON
    -DFMT_DOC=OFF
    -DFMT_TEST=OFF)

ExternalProject_Add(zlib
  URL https://zlib.net/zlib-1.2.11.tar.gz
  URL_MD5 1c9f62f0778697a09d36121ead88e08e
  INSTALL_DIR @SMF_DEPS_INSTALL_DIR@
  CONFIGURE_COMMAND
    <SOURCE_DIR>/configure
    --prefix=<INSTALL_DIR>
  BUILD_COMMAND ${make_command}
  INSTALL_COMMAND ${make_command} install)


ExternalProject_Add(protobuf
  DEPENDS zlib
  URL https://github.com/protocolbuffers/protobuf/archive/v3.7.0.tar.gz
  URL_MD5 99ab003ca0e98c9dc40edbd60dd43633
  INSTALL_DIR   @SMF_DEPS_INSTALL_DIR@
  SOURCE_SUBDIR cmake
  CMAKE_ARGS
    -Dprotobuf_BUILD_TESTS=OFF
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>)

set(dpdk_args
  -Dc_args="-Wno-error"
  -Denable_docs=false
  -Dtests=false
  -Dexamples=
  -Datomic_mbuf_ref_counts=false
  -Dmax_memseg_lists=8192
  -Ddisable_drivers="net/softnic,net/bonding"
  -Ddisable_libs="kni,jobstats,lpm,acl,power,ip_frag,distributor,reorder,port,table,pipeline,flow_classify,bpf,efd,member"
  -Dcpu_instruction_set=native)

find_program(Meson_EXECUTABLE meson)
if(NOT Meson_EXECUTABLE)
  message(FATAL_ERROR "Cooking: Meson is required!")
endif()

find_program(Ninja_EXECUTABLE ninja)
if(NOT Ninja_EXECUTABLE)
  message(FATAL_ERROR "Cooking: Ninja is required!")
endif()

ExternalProject_Add(dpdk
  URL https://github.com/scylladb/dpdk/archive/826e11d55516f53c58592ac4c4a052a80595067c.tar.gz
  URL_MD5 70b7d007087596b1cf741f8c14ddea46
  INSTALL_DIR    @SMF_DEPS_INSTALL_DIR@
  CONFIGURE_COMMAND
    env CC=@CMAKE_C_COMPILER@ ${Meson_EXECUTABLE} ${dpdk_args} --prefix=<INSTALL_DIR> <BINARY_DIR> <SOURCE_DIR>
  BUILD_COMMAND
    ${Ninja_EXECUTABLE} -C <BINARY_DIR>
  INSTALL_COMMAND
    ${Ninja_EXECUTABLE} -C <BINARY_DIR> install)

ExternalProject_Add(c-ares
  URL https://github.com/c-ares/c-ares/archive/cares-1_14_0.tar.gz
  URL_MD5 79dcc4f2adaff431920755d9cafc9933
  INSTALL_DIR @SMF_DEPS_INSTALL_DIR@
  CMAKE_ARGS
    -DCARES_VERSION="1.14.0"
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DCARES_INSTALL=ON)

ExternalProject_Add(yaml-cpp
  URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.6.0.tar.gz
  URL_MD5 8adc0ae6c2698a61ab086606cc7cf562
  INSTALL_DIR    @SMF_DEPS_INSTALL_DIR@
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DYAML_CPP_BUILD_TOOLS=OFF
    -DYAML_CPP_BUILD_TESTS=OFF
    -DBUILD_SHARED_LIBS=OFF
  DEPENDS boost)

ExternalProject_Add(gmp
  URL https://gmplib.org/download/gmp/gmp-6.1.2.tar.bz2
  URL_MD5 8ddbb26dc3bd4e2302984debba1406a5
  INSTALL_DIR @SMF_DEPS_INSTALL_DIR@
  CONFIGURE_COMMAND
    <SOURCE_DIR>/configure
    --prefix=<INSTALL_DIR>
    --srcdir=<SOURCE_DIR>
    ${info_dir}/gmp
  BUILD_COMMAND ${make_command}
  INSTALL_COMMAND ${make_command} install)

ExternalProject_Add(nettle
  URL https://ftp.gnu.org/gnu/nettle/nettle-3.7.tar.gz
  URL_MD5 568070f62fd14ca6b076ad583d84e9db
  INSTALL_DIR @SMF_DEPS_INSTALL_DIR@
  CONFIGURE_COMMAND
    <SOURCE_DIR>/configure
    --prefix=<INSTALL_DIR>
    --srcdir=<SOURCE_DIR>
    --libdir=<INSTALL_DIR>/lib
    ${info_dir}/nettle
    ${autotools_flags}
  BUILD_COMMAND ${make_command}
  INSTALL_COMMAND ${make_command} install
  DEPENDS gmp)

ExternalProject_Add(gnutls
  URL https://www.gnupg.org/ftp/gcrypt/gnutls/v3.7/gnutls-3.7.1.tar.xz  
  URL_MD5 278e1f50d79cd13727733adbf01fde8f
  INSTALL_DIR @SMF_DEPS_INSTALL_DIR@
  CONFIGURE_COMMAND
   ${CMAKE_COMMAND} -E env ${PKG_CONFIG_PATH}
    <SOURCE_DIR>/configure
    --prefix=<INSTALL_DIR>
    --srcdir=<SOURCE_DIR>
    --with-included-unistring
    --with-included-libtasn1
    --without-idn
    --without-p11-kit
    # https://lists.gnupg.org/pipermail/gnutls-help/2016-February/004085.html
    --disable-non-suiteb-curves
    --disable-doc
    ${autotools_flags}
  BUILD_COMMAND ${make_command}
  INSTALL_COMMAND ${make_command} install
  DEPENDS gmp nettle)

ExternalProject_Add(lz4
  URL https://github.com/lz4/lz4/archive/v1.8.3.tar.gz
  URL_MD5 d5ce78f7b1b76002bbfffa6f78a5fc4e
  INSTALL_DIR   @SMF_DEPS_INSTALL_DIR@
  SOURCE_SUBDIR contrib/cmake_unofficial
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>)

ExternalProject_Add(numactl
  URL https://github.com/numactl/numactl/releases/download/v2.0.12/numactl-2.0.12.tar.gz
  URL_MD5 2ba9777d78bfd7d408a387e53bc33ebc
  INSTALL_DIR @SMF_DEPS_INSTALL_DIR@
  CONFIGURE_COMMAND
    <SOURCE_DIR>/configure
    --prefix=<INSTALL_DIR>
    --srcdir=<SOURCE_DIR>
  BUILD_COMMAND ${make_command}
  INSTALL_COMMAND ${make_command} install)

ExternalProject_Add(libpciaccess
  URL https://www.x.org/releases/individual/lib/libpciaccess-0.13.4.tar.gz
  URL_MD5 cc1fad87da60682af1d5fa43a5da45a4
  INSTALL_DIR @SMF_DEPS_INSTALL_DIR@
  CONFIGURE_COMMAND
    <SOURCE_DIR>/configure
    --prefix=<INSTALL_DIR>
    --srcdir=<SOURCE_DIR>
  BUILD_COMMAND ${make_command}
  INSTALL_COMMAND ${make_command} install)

ExternalProject_Add(hwloc
  URL https://download.open-mpi.org/release/hwloc/v2.2/hwloc-2.2.0.tar.gz
  URL_MD5 762c93cdca3249eed4627c4a160192bd
  INSTALL_DIR @SMF_DEPS_INSTALL_DIR@
  CONFIGURE_COMMAND
    <SOURCE_DIR>/configure
    --prefix=<INSTALL_DIR>
    --srcdir=<SOURCE_DIR>
  BUILD_COMMAND ${make_command}
  INSTALL_COMMAND ${make_command} install
  DEPENDS numactl libpciaccess)

ExternalProject_Add(xz
  URL https://github.com/xz-mirror/xz/releases/download/v5.2.2/xz-5.2.2.tar.gz
  URL_MD5 7cf6a8544a7dae8e8106fdf7addfa28c
  INSTALL_DIR @SMF_DEPS_INSTALL_DIR@
  CONFIGURE_COMMAND
    COMMAND
      <SOURCE_DIR>/configure
      --prefix=<INSTALL_DIR>
  BUILD_COMMAND ${make_command}
  INSTALL_COMMAND ${make_command} install)

ExternalProject_Add(xml2
  URL https://github.com/GNOME/libxml2/archive/f8a8c1f59db355b46962577e7b74f1a1e8149dc6.tar.gz
  URL_MD5 8717ac0b233af3ed1e47236afa6e7946
  INSTALL_DIR @SMF_DEPS_INSTALL_DIR@
  CONFIGURE_COMMAND
    COMMAND
      ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>
      ${CMAKE_COMMAND} -E env NOCONFIGURE=true ./autogen.sh
    COMMAND
      <SOURCE_DIR>/configure
      --without-python
      --prefix=<INSTALL_DIR>
  BUILD_COMMAND ${make_command}
  INSTALL_COMMAND ${make_command} install
  DEPENDS xz zlib)

ExternalProject_Add(cryptopp
  URL https://github.com/weidai11/cryptopp/archive/CRYPTOPP_5_6_5.tar.gz
  URL_MD5 88224d9c0322f63aa1fb5b8ae78170f0
  INSTALL_DIR @SMF_DEPS_INSTALL_DIR@
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DBUILD_TESTING=OFF)

ExternalProject_Add(lksctp-tools
  # Note: 1.0.18 has a bug w/ the build/configuration system that does not
  # produce sctp.h.in -> sctp.h and instead install sctp.h.in
  URL https://github.com/sctp/lksctp-tools/archive/lksctp-tools-1.0.17.tar.gz
  URL_MD5 910a4f1d4024d71149b91f50f97eae23
  INSTALL_DIR @SMF_DEPS_INSTALL_DIR@
  PATCH_COMMAND ./bootstrap
  CONFIGURE_COMMAND
    <SOURCE_DIR>/configure
    --prefix=<INSTALL_DIR>
    --srcdir=<SOURCE_DIR>
  BUILD_COMMAND ${make_command}
  INSTALL_COMMAND ${make_command} install)

# Set PKG_CONFIG_PATH environment variable to help seastar locate dpdk
set(ENV{PKG_CONFIG_PATH} "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/:$ENV{PKG_CONFIG_PATH}")
message(STATUS "PKG_CONFIG_PATH=$ENV{PKG_CONFIG_PATH}")

ExternalProject_Add(seastar
  URL https://github.com/scylladb/seastar/archive/d82946d45e46b1f01d658a8543632be32839d75b.tar.gz
  INSTALL_DIR    @SMF_DEPS_INSTALL_DIR@
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DSeastar_INSTALL=ON
    -DSeastar_HWLOC=@SMF_ENABLE_HWLOC@
    -DSeastar_DPDK=@SMF_ENABLE_DPDK@
    -DSeastar_DPDK_MACHINE=@SMF_DPDK_MACHINE@
    -DSeastar_APPS=OFF
    -DSeastar_DEMOS=OFF
    -DSeastar_DOCS=OFF
    -DSeastar_TESTING=OFF
    -DSeastar_CXX_FLAGS=-Wno-error
    -DSeastar_LD_FLAGS=-pthread
    -DSeastar_API_LEVEL=6
    -DSeastar_CXX_DIALECT=c++17
    -DSeastar_UNUSED_RESULT_ERROR=ON
  DEPENDS
    fmt
    boost
    protobuf
    dpdk
    yaml-cpp
    c-ares
    gnutls
    lz4
    hwloc
    numactl
    xml2
    xz
    cryptopp
    lksctp-tools
    zlib)

ExternalProject_Add(zstd
  URL https://github.com/facebook/zstd/archive/470344d33e1d52a2ada75d278466da8d4ee2faf6.tar.gz
  URL_MD5 52368c486e11c0f21990c8f2d4b556f9
  INSTALL_DIR    @SMF_DEPS_INSTALL_DIR@
  SOURCE_SUBDIR  build/cmake
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DZSTD_MULTITHREAD_SUPPORT=OFF
    -DZSTD_LEGACY_SUPPORT=OFF
    -DZSTD_BUILD_STATIC=ON
    -DZSTD_BUILD_SHARED=OFF
    -DZSTD_BUILD_PROGRAMS=OFF)

ExternalProject_Add(HdrHistogram
  URL https://storage.googleapis.com/vectorizedio-public/dependencies/HdrHistogram_c-0.11.2.tar.gz
  URL_MD5 95970dea64f1a7a8d199aeb9f7c15e60
  INSTALL_DIR    @SMF_DEPS_INSTALL_DIR@
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DHDR_HISTOGRAM_BUILD_PROGRAMS=OFF
    -DHDR_HISTOGRAM_BUILD_SHARED=OFF
  DEPENDS zlib)

ExternalProject_Add(xxhash
  URL https://github.com/Cyan4973/xxHash/archive/c9970b8ec4155e789f1c3682da923869a496ba9d.tar.gz
  URL_MD5 c1c87865e98f79144814a1d57db47751
  INSTALL_DIR    @SMF_DEPS_INSTALL_DIR@
  SOURCE_SUBDIR  cmake_unofficial
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DBUILD_XXHSUM=OFF
    -DBUILD_SHARED_LIBS=OFF
    -DBUILD_ENABLE_INLINE_API=ON)


ExternalProject_Add(gtest
  URL https://github.com/google/googletest/archive/644319b9f06f6ca9bf69fe791be399061044bc3d.tar.gz
  URL_MD5 e4cfcbe80bf32cb8caa8a7666ebc206c
  INSTALL_DIR    @SMF_DEPS_INSTALL_DIR@
  CMAKE_ARGS
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>)

ExternalProject_Add(gflags
  URL https://github.com/gflags/gflags/archive/1005485222e8b0feff822c5723ddcaa5abadc01a.tar.gz
  URL_MD5 25582b81561bb2f1d2c05e05b4b566a8
  INSTALL_DIR    @SMF_DEPS_INSTALL_DIR@
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DBUILD_SHARED_LIBS=OFF
    -DBUILD_STATIC_LIBS=ON
    -DBUILD_gflags_LIB=ON
    -DBUILD_TESTING=OFF
    -DBUILD_PACKAGING=OFF)

ExternalProject_Add(glog
  URL https://github.com/google/glog/archive/5d46e1bcfc92bf06a9ca3b3f1c5bb1dc024d9ecd.tar.gz
  URL_MD5 548900b10c18b2b7a43ae8960ec3a777
  INSTALL_DIR    @SMF_DEPS_INSTALL_DIR@
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DBUILD_SHARED_LIBS=OFF
    -DWITH_GFLAGS=ON
  DEPENDS gflags)

ExternalProject_Add(googlebenchmark
  URL https://github.com/google/benchmark/archive/f91b6b42b1b9854772a90ae9501464a161707d1e.tar.gz
  URL_MD5 4d8d16168d20bdb4837990b18abdcd24
  INSTALL_DIR    @SMF_DEPS_INSTALL_DIR@
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
    -DBENCHMARK_ENABLE_TESTING=OFF
  DEPENDS gflags)

# note about master
ExternalProject_Add(flatbuffers
  URL https://github.com/google/flatbuffers/archive/6df40a2471737b27271bdd9b900ab5f3aec746c7.tar.gz
  URL_MD5 633d21f362f9c636c27667c246464631
  INSTALL_DIR    @SMF_DEPS_INSTALL_DIR@
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@
    -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DFLATBUFFERS_INSTALL=ON
    -DFLATBUFFERS_BUILD_FLATC=ON
    -DFLATBUFFERS_BUILD_TESTS=OFF
    -DFLATBUFFERS_BUILD_FLATHASH=OFF)
