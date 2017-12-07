add_library (smf_filesystem STATIC
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_writer_node.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_writer.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/write_ahead_log.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_clock_pro_cache.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_reader.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_reader_node.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/file_size_utils.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_name_extractor_utils.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_name_extractor_utils.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_writer_utils.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_pretty_print_utils.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_opts.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_writer_file_lease.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_write_projection.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_partition_manager.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_write_behind_cache.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_lcore_map.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_topics_manager.cc
  )

# add_dependencies(smf_filesystem
#   ${PROJECT_SOURCE_DIR}/src/flatbuffers/wal_generated.fb.h)
target_link_libraries (smf_filesystem
  smf_utils
  smf_tracing
  xxhash_static
  zstd.a
  re2.a
  libroaring.a
  )

install(TARGETS smf_filesystem DESTINATION lib)
