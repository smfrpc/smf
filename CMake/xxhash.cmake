include(CMake/base_flags.cmake)
include_directories(
  SYSTEM
  ${PROJECT_SOURCE_DIR}/meta/tmp/xxhash
  )
add_library(xxhash_static STATIC
  ${PROJECT_SOURCE_DIR}/meta/tmp/xxhash/xxhash.c
  )
target_link_libraries(xxhash_static ${BASE_FLAGS})
