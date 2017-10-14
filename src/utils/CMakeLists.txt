add_library (
  smf_utils STATIC
  ${PROJECT_SOURCE_DIR}/src/utils/random.cc
  ${PROJECT_SOURCE_DIR}/src/utils/checks/cpu.cc
  ${PROJECT_SOURCE_DIR}/src/utils/checks/memory.cc
  ${PROJECT_SOURCE_DIR}/src/utils/checks/disk.cc
  ${PROJECT_SOURCE_DIR}/src/utils/human_bytes_printing_utils.cc
  ${PROJECT_SOURCE_DIR}/src/utils/compression.cc
  )
target_link_libraries (
  smf_utils
  xxhash_static
  zstd.a
  lz4.a
  )
smf_seastar_target(smf_utils)
install(TARGETS smf_utils DESTINATION lib)
