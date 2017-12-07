add_library (
  smf_utils STATIC
  ${PROJECT_SOURCE_DIR}/src/utils/random.cc
  ${PROJECT_SOURCE_DIR}/src/utils/checks/cpu.cc
  ${PROJECT_SOURCE_DIR}/src/utils/checks/memory.cc
  ${PROJECT_SOURCE_DIR}/src/utils/checks/disk.cc
  ${PROJECT_SOURCE_DIR}/src/utils/human_bytes.cc
  ${PROJECT_SOURCE_DIR}/src/utils/compression.cc
  )
target_link_libraries (
  smf_utils
#  xxhash_static
  zstd.a
  lz4.a
  )
smf_seastar_target(smf_utils)
install(TARGETS smf_utils DESTINATION lib)
install(FILES
  ${PROJECT_SOURCE_DIR}/src/utils/caching/clock_pro/chunk.h
  ${PROJECT_SOURCE_DIR}/src/utils/caching/clock_pro/clock_list.h
  ${PROJECT_SOURCE_DIR}/src/utils/caching/clock_pro/clock_pro.h
  DESTINATION include/smf/utils/caching
  )
install(FILES
  ${PROJECT_SOURCE_DIR}/src/utils/checks/cpu.h
  ${PROJECT_SOURCE_DIR}/src/utils/checks/disk.h
  ${PROJECT_SOURCE_DIR}/src/utils/checks/memory.h
  DESTINATION include/smf/utils/checks
  )
install(FILES
  ${PROJECT_SOURCE_DIR}/src/utils/compression.h
  ${PROJECT_SOURCE_DIR}/src/utils/human_bytes.h
  ${PROJECT_SOURCE_DIR}/src/utils/random.h
  ${PROJECT_SOURCE_DIR}/src/utils/time_utils.h
  DESTINATION include/smf/utils
  )
