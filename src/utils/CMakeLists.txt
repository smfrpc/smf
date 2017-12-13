add_library(smf_utils STATIC
  random.cc
  checks/cpu.cc
  checks/memory.cc
  checks/disk.cc
  human_bytes_printing_utils.cc
  compression.cc)
target_link_libraries (smf_utils
  xxhash
  libzstd_static
  lz4
  Seastar::seastar)
target_include_directories(smf_utils
  PRIVATE ${ZSTD_INCLUDE_DIR})
install(TARGETS smf_utils DESTINATION lib)

install(FILES
  caching/clock_pro/chunk.h
  caching/clock_pro/clock_list.h
  caching/clock_pro/clock_pro.h
  DESTINATION include/smf/utils/caching/clock_pro
  )
install(FILES
  checks/cpu.h
  checks/disk.h
  checks/memory.h
  DESTINATION include/smf/utils/checks
  )
install(FILES
  compression.h
  human_bytes_printing_utils.h
  random.h
  time_utils.h
  DESTINATION include/smf/utils
  )
