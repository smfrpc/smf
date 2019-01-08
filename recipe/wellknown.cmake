# Useful definitions for `cmake -E env`.
set (amended_PATH PATH=${Cooking_INGREDIENTS_DIR}/bin:$ENV{PATH})
set (PKG_CONFIG_PATH PKG_CONFIG_PATH=${Cooking_INGREDIENTS_DIR}/lib/pkgconfig)
# Some Autotools ingredients need this information because they don't use pkgconfig.
set (autotools_ingredients_flags
  CFLAGS=-I${Cooking_INGREDIENTS_DIR}/include
  CXXFLAGS=-I${Cooking_INGREDIENTS_DIR}/include
  LDFLAGS=-L${Cooking_INGREDIENTS_DIR}/lib)
# Some Autotools projects amend the info file instead of making a package-specific one.
#     ${info_dir}/gmp
set (info_dir --infodir=<INSTALL_DIR>/share/info)
cmake_host_system_information (
  RESULT build_concurrency_factor
  QUERY NUMBER_OF_LOGICAL_CORES)
set (make_command make -j ${build_concurrency_factor})

##
## Dependencies of dependencies of dependencies.
##

cooking_ingredient (gmp
  EXTERNAL_PROJECT_ARGS
    URL https://gmplib.org/download/gmp/gmp-6.1.2.tar.bz2
    URL_MD5 8ddbb26dc3bd4e2302984debba1406a5
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --srcdir=<SOURCE_DIR> ${info_dir}/gmp
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND ${make_command} install)

##
## Dependencies of dependencies.
##

cooking_ingredient (colm
  EXTERNAL_PROJECT_ARGS
    URL http://www.colm.net/files/colm/colm-0.13.0.6.tar.gz
    URL_MD5 16aaf566cbcfe9a06154e094638ac709
    # This is upsetting.
    BUILD_IN_SOURCE YES
    CONFIGURE_COMMAND ./configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND ${make_command} install)

cooking_ingredient (libpciaccess
  EXTERNAL_PROJECT_ARGS
    URL https://www.x.org/releases/individual/lib/libpciaccess-0.13.4.tar.gz
    URL_MD5 cc1fad87da60682af1d5fa43a5da45a4
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --srcdir=<SOURCE_DIR>
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND ${make_command} install)

cooking_ingredient (nettle
  REQUIRES gmp
  EXTERNAL_PROJECT_ARGS
    URL https://ftp.gnu.org/gnu/nettle/nettle-3.4.tar.gz
    URL_MD5 dc0f13028264992f58e67b4e8915f53d
    CONFIGURE_COMMAND
      <SOURCE_DIR>/configure
      --prefix=<INSTALL_DIR>
      --srcdir=<SOURCE_DIR>
      --libdir=<INSTALL_DIR>/lib
      ${info_dir}/nettle
      ${autotools_ingredients_flags}
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND ${make_command} install)

# Also a direct dependency of Seastar.
cooking_ingredient (numactl
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/numactl/numactl/releases/download/v2.0.12/numactl-2.0.12.tar.gz
    URL_MD5 2ba9777d78bfd7d408a387e53bc33ebc
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --srcdir=<SOURCE_DIR>
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND ${make_command} install)

cooking_ingredient (zlib
  EXTERNAL_PROJECT_ARGS
    URL https://zlib.net/zlib-1.2.11.tar.gz
    URL_MD5 1c9f62f0778697a09d36121ead88e08e
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --64
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND ${make_command} install)

##
## Private and private/public dependencies.
##

cooking_ingredient (Boost
  EXTERNAL_PROJECT_ARGS
    # The 1.67.0 release has a bug in Boost Lockfree around a missing header.
    URL https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.gz
    URL_MD5 319c6ffbbeccc366f14bb68767a6db79
    PATCH_COMMAND
      ./bootstrap.sh
      --prefix=<INSTALL_DIR>
      --with-libraries=atomic,chrono,date_time,filesystem,program_options,system,test,thread
    CONFIGURE_COMMAND <DISABLE>
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND
      ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>
      ./b2
      -j ${build_concurrency_factor}
      --layout=system
      --build-dir=<BINARY_DIR>
      install
      variant=debug
      link=static # different from seastar
      threading=multi
      hardcode-dll-paths=true
      dll-path=<INSTALL_DIR>/lib)

cooking_ingredient (GnuTLS
  REQUIRES
    gmp
    nettle
  EXTERNAL_PROJECT_ARGS
    URL https://www.gnupg.org/ftp/gcrypt/gnutls/v3.5/gnutls-3.5.18.tar.xz
    URL_MD5 c2d93d305ecbc55939bc2a8ed4a76a3d
    CONFIGURE_COMMAND
     ${CMAKE_COMMAND} -E env ${PKG_CONFIG_PATH}
      <SOURCE_DIR>/configure
      --prefix=<INSTALL_DIR>
      --srcdir=<SOURCE_DIR>
      --with-included-unistring
      --with-included-libtasn1
      --without-p11-kit
      # https://lists.gnupg.org/pipermail/gnutls-help/2016-February/004085.html
      --disable-non-suiteb-curves
      --disable-doc
      ${autotools_ingredients_flags}
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND ${make_command} install)

cooking_ingredient (Protobuf
  REQUIRES zlib
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/protocolbuffers/protobuf/releases/download/v3.3.0/protobuf-cpp-3.3.0.tar.gz
    URL_MD5 73c28d3044e89782bdc8d9fdcfbb5792
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --srcdir=<SOURCE_DIR>
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND ${make_command} install)

cooking_ingredient (hwloc
  REQUIRES
    numactl
    libpciaccess
  EXTERNAL_PROJECT_ARGS
    URL https://download.open-mpi.org/release/hwloc/v1.11/hwloc-1.11.5.tar.gz
    URL_MD5 8f5fe6a9be2eb478409ad5e640b2d3ba
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --srcdir=<SOURCE_DIR>
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND ${make_command} install)

cooking_ingredient (ragel
  REQUIRES colm
  EXTERNAL_PROJECT_ARGS
    URL http://www.colm.net/files/ragel/ragel-6.10.tar.gz
    URL_MD5 748cae8b50cffe9efcaa5acebc6abf0d
    # This is upsetting.
    BUILD_IN_SOURCE YES
    CONFIGURE_COMMAND
      ${CMAKE_COMMAND} -E env ${amended_PATH}
      ./configure
      --prefix=<INSTALL_DIR>
      # This is even more upsetting.
      ${autotools_ingredients_flags}
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND ${make_command} install)

cooking_ingredient (lksctp-tools
  EXTERNAL_PROJECT_ARGS
    URL https://sourceforge.net/projects/lksctp/files/lksctp-tools/lksctp-tools-1.0.16.tar.gz
    URL_MD5 708bb0b5a6806ad6e8d13c55b067518e
    PATCH_COMMAND ./bootstrap
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --srcdir=<SOURCE_DIR>
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND ${make_command} install)

cooking_ingredient (yaml-cpp
  REQUIRES Boost
  CMAKE_ARGS
    -DYAML_CPP_BUILD_TESTS=OFF
    -DBUILD_SHARED_LIBS=OFF # different from seastar
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.5.3.tar.gz
    URL_MD5 2bba14e6a7f12c7272f87d044e4a7211)

##
## Public dependencies.
##

cooking_ingredient (c-ares
  EXTERNAL_PROJECT_ARGS
    URL https://c-ares.haxx.se/download/c-ares-1.13.0.tar.gz
    URL_MD5 d2e010b43537794d8bedfb562ae6bba2
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --srcdir=<SOURCE_DIR>
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND ${make_command} install)

cooking_ingredient (cryptopp
  CMAKE_ARGS
    -DCMAKE_INSTALL_LIBDIR=<INSTALL_DIR>/lib
    -DBUILD_TESTING=OFF
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/weidai11/cryptopp/archive/CRYPTOPP_5_6_5.tar.gz
    URL_MD5 88224d9c0322f63aa1fb5b8ae78170f0)

set (dpdk_quadruple ${CMAKE_SYSTEM_PROCESSOR}-native-linuxapp-gcc)

set (dpdk_args
  EXTRA_CFLAGS=-Wno-error
  O=<BINARY_DIR>
  DESTDIR=<INSTALL_DIR>
  T=${dpdk_quadruple})

# TODO(agallego) - change for upstream DPDK
cooking_ingredient (dpdk
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/scylladb/dpdk/archive/cc7e6ed.tar.gz
    URL_MD5 7aa07a7138e8d1b774cc5db2bd930d0d
    CONFIGURE_COMMAND
      COMMAND
        ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>
        make ${dpdk_args} config
      COMMAND
        ${CMAKE_COMMAND}
        -DSeastar_DPDK_CONFIG_FILE_IN=<BINARY_DIR>/.config
        -DSeastar_DPDK_CONFIG_FILE_CHANGES=${CMAKE_CURRENT_LIST_DIR}/dpdk_config
        -DSeastar_DPDK_CONFIG_FILE_OUT=<BINARY_DIR>/${dpdk_quadruple}/.config
        -P ${CMAKE_CURRENT_LIST_DIR}/dpdk_configure.cmake
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND
      ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>
      ${make_command} ${dpdk_args} install)

cooking_ingredient (fmt
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS=OFF #different from seastar
    -DFMT_DOC=OFF
    -DFMT_LIB_DIR=lib
    -DFMT_TEST=OFF
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/fmtlib/fmt/archive/5.3.0.tar.gz
    URL_MD5 1015bf3ff2a140dfe03de50ee2469401)

cooking_ingredient (lz4
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/lz4/lz4/archive/v1.8.0.tar.gz
    URL_MD5 6247bf0e955899969d1600ff34baed6b
    # This is upsetting.
    BUILD_IN_SOURCE ON
    CONFIGURE_COMMAND <DISABLE>
    BUILD_COMMAND <DISABLE>
    INSTALL_COMMAND ${make_command} PREFIX=<INSTALL_DIR> install)

cooking_ingredient(Seastar
  REQUIRES
    Boost
    fmt
    lz4
    cryptopp
    c-ares
    dpdk
  # This means in seastar/recipe/dev.cmake
  # COOKING_RECIPE dev
  COOKING_CMAKE_ARGS
    # Not `lib64`.
    -DCMAKE_INSTALL_LIBDIR=lib
    -DSeastar_APPS=OFF
    -DSeastar_DOCS=OFF
    -DSeastar_DEMOS=OFF
    -DSeastar_DPDK=ON
    -DSeastar_TESTING=OFF
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/scylladb/seastar/archive/5e39990.tar.gz
    URL_MD5 9e57a4ca35493e375bef68288c6424ce)


# ------------- additional dependencies after seastar


cooking_ingredient(Hdrhistogram
  REQUIRES zlib
  CMAKE_ARGS
    -DHDR_HISTOGRAM_BUILD_SHARED=OFF
    -DHDR_HISTOGRAM_BUILD_SHARED=OFF
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/HdrHistogram/HdrHistogram_c/archive/59cbede.tar.gz
    URL_MD5 6794ddea304a7e0e939ad807a70fea5b)

cooking_ingredient(Zstd
  CMAKE_ARGS
    -DZSTD_MULTITHREAD_SUPPORT=OFF
    -DZSTD_LEGACY_SUPPORT=OFF
    -DZSTD_BUILD_STATIC=ON
    -DZSTD_BUILD_SHARED=OFF
    -DZSTD_BUILD_PROGRAMS=OFF
  EXTERNAL_PROJECT_ARGS
    SOURCE_SUBDIR build/cmake
    URL https://github.com/facebook/zstd/archive/c5be47c.tar.gz
    URL_MD5 ae474660d44f7fdd24c91e1aa0ba572c)

cooking_ingredient(Flatbuffers
  CMAKE_ARGS
    -DFLATBUFFERS_BUILD_TESTS=OFF
    -DFLATBUFFERS_INSTALL=OFF
    -DFLATBUFFERS_BUILD_FLATHASH=OFF
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/google/flatbuffers/archive/b99332e.tar.gz
    URL_MD5 e1d499e557f4e3299ca5379d84d40686)

set(
  FLATBUFFERS_COMPILER
  ${Flatbuffers_BINARY_DIR}/flatc
  CACHE
  STRING
  "flatc binary for generating code"
  FORCE)


cooking_ingredient(Googletest
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/google/googletest/archive/644319b.tar.gz
    URL_MD5 e4cfcbe80bf32cb8caa8a7666ebc206c)

cooking_ingredient(Googlebenchmark
  CMAKE_ARGS
    -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
    -DBENCHMARK_ENABLE_TESTING=OFF
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/google/benchmark/archive/eec9a8e.tar.gz
    URL_MD5 81f6af37a81753cb9d799012b62ae04d)
