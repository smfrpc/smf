add_library (smf_filesystem STATIC
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_writer_node.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_writer.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_clock_pro_cache.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_reader.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_reader_node.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_impl_with_cache.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/file_size_utils.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_name_extractor_utils.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_name_extractor_utils.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_writer_utils.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_pretty_print_utils.cc
  )

# add_dependencies(smf_filesystem
#   ${PROJECT_SOURCE_DIR}/src/flatbuffers/wal_generated.fb.h)
target_link_libraries (smf_filesystem
  ${SEASTAR_LDFLAGS}
  smftracing
  xxhash_static
  zstd.a
  re2
  libroaring.a
  )

install(TARGETS smf_filesystem DESTINATION lib)
