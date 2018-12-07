if(SEASTAR_ENABLE_DPDK)
  set(SEASTAR_ENABLE_DPDK_OPTION ON)
else()
  set(SEASTAR_ENABLE_DPDK_OPTION OFF)
endif()

cooking_ingredient(Seastar
  COOKING_RECIPE dev
  COOKING_CMAKE_ARGS
    -DSeastar_DPDK=${SEASTAR_ENABLE_DPDK_OPTION}
    -DSeastar_APPS=OFF
    -DSeastar_DEMOS=OFF
    -DSeastar_DOCS=OFF
    -DSeastar_TESTING=OFF
    -DSeastar_CXX_FLAGS=-Wno-stringop-overflow;-Wno-array-bounds;-Wno-stringop-truncation;-Wno-format-overflow
  EXTERNAL_PROJECT_ARGS
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/third_party/seastar)
