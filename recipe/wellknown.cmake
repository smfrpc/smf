cmake_host_system_information (
  RESULT build_concurrency_factor
  QUERY NUMBER_OF_LOGICAL_CORES)
set (make_command make -j ${build_concurrency_factor})

# we have transitive dependencies. Seastar *must* come first
cooking_ingredient(Seastar
  REQUIRES Boost
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
    URL https://github.com/scylladb/seastar/archive/5e39990.tar.gz
    URL_MD5 9e57a4ca35493e375bef68288c6424ce)

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
  ${flatbuffers_BINARY_DIR}/flatc
  CACHE
  STRING
  "flatc binary for generating code"
  FORCE)
