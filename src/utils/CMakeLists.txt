add_library(smf_utils STATIC
  random.cc
  checks/cpu.cc
  checks/memory.cc
  checks/disk.cc
  human_bytes.cc
  compression.cc)
target_link_libraries(smf_utils
  libzstd_static
  lz4
  seastar)
target_include_directories(smf_utils
  PRIVATE ${ZSTD_INCLUDE_DIR})

install(TARGETS smf_utils DESTINATION lib)

install(FILES
  checks/cpu.h
  checks/disk.h
  checks/memory.h
  DESTINATION include/smf/utils/checks)

install(FILES
  compression.h
  human_bytes.h
  random.h
  time_utils.h
  DESTINATION include/smf/utils)
