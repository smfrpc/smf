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

cooking_ingredient (yaml-cpp
  REQUIRES Boost
  CMAKE_ARGS
    -DYAML_CPP_BUILD_TOOLS=OFF
    -DBUILD_SHARED_LIBS=OFF # different from seastar
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.5.3.tar.gz
    URL_MD5 2bba14e6a7f12c7272f87d044e4a7211
    )

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
    zlib
    Boost
    yaml-cpp
    fmt
    lz4
  # This means in seastar/recipe/dev.cmake
  COOKING_RECIPE dev
  COOKING_CMAKE_ARGS
    # Not `lib64`.
    -DCMAKE_INSTALL_LIBDIR=lib
    -DSeastar_APPS=OFF
    -DSeastar_DOCS=OFF
    -DSeastar_DEMOS=OFF
    -DSeastar_DPDK=ON
    -DSeastar_TESTING=OFF
  EXTERNAL_PROJECT_ARGS
    GIT_REPOSITORY https://github.com/scylladb/seastar/
    GIT_TAG c9d6e20
    )


# ------------- additional dependencies after seastar


cooking_ingredient(Hdrhistogram
  REQUIRES zlib
  CMAKE_ARGS
    -DHDR_HISTOGRAM_BUILD_PROGRAMS=OFF
    -DHDR_HISTOGRAM_BUILD_SHARED=OFF
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/HdrHistogram/HdrHistogram_c/archive/59cbede.tar.gz
    URL_MD5 6794ddea304a7e0e939ad807a70fea5b)

cooking_ingredient(Zstd
  EXTERNAL_PROJECT_ARGS
    CONFIGURE_COMMAND
      ${CMAKE_COMMAND}
      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
      -DZSTD_MULTITHREAD_SUPPORT=OFF
      -DZSTD_LEGACY_SUPPORT=OFF
      -DZSTD_BUILD_STATIC=ON
      -DZSTD_BUILD_SHARED=OFF
      -DZSTD_BUILD_PROGRAMS=OFF
      <SOURCE_DIR>/build/cmake
    URL https://github.com/facebook/zstd/archive/c5be47c.tar.gz
    URL_MD5 ae474660d44f7fdd24c91e1aa0ba572c)

cooking_ingredient(Flatbuffers
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=Release
    -DFLATBUFFERS_INSTALL=ON
    -DFLATBUFFERS_BUILD_FLATC=ON
    -DFLATBUFFERS_BUILD_TESTS=OFF
    -DFLATBUFFERS_BUILD_FLATHASH=OFF
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/google/flatbuffers/archive/b99332e.tar.gz
    URL_MD5 e1d499e557f4e3299ca5379d84d40686)

cooking_ingredient(GFlags
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=Release
    -DGFLAGS_NAMESPACE=google
    -DBUILD_SHARED_LIBS=ON
    -DBUILD_STATIC_LIBS=ON
    -DBUILD_gflags_LIB=ON
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/gflags/gflags/archive/1005485.tar.gz)

cooking_ingredient(Googletest
  REQUIRES GFlags
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/google/googletest/archive/644319b.tar.gz
    URL_MD5 e4cfcbe80bf32cb8caa8a7666ebc206c)

cooking_ingredient(Googlebenchmark
  REQUIRES GFlags
  CMAKE_ARGS
    -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
    -DBENCHMARK_ENABLE_TESTING=OFF
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/google/benchmark/archive/eec9a8e.tar.gz
    URL_MD5 81f6af37a81753cb9d799012b62ae04d)

cooking_ingredient(Glog
  REQUIRES GFlags
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=Release
    -DBUILD_SHARED_LIBS=ON
    -DWITH_GFLAGS=ON
  EXTERNAL_PROJECT_ARGS
    URL https://github.com/google/glog/archive/5d46e1b.tar.gz)

cooking_ingredient (xxHash
  EXTERNAL_PROJECT_ARGS
    CONFIGURE_COMMAND
      ${CMAKE_COMMAND}
      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
      -DBUILD_XXHSUM=OFF
      -DBUILD_SHARED_LIBS=OFF
      <SOURCE_DIR>/cmake_unofficial
    INSTALL_COMMAND
      ${make_command} install && ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/xxhash.c <INSTALL_DIR>/include/xxhash.c
    URL https://github.com/Cyan4973/xxHash/archive/v0.6.5.tar.gz
    URL_MD5 6af3a964f3c2accebce66e54b44b6446)
