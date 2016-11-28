add_library (smf_filesystem STATIC
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_writer_node.cc
  ${PROJECT_SOURCE_DIR}/src/filesystem/wal_writer.cc
  )
# add_dependencies(smf_filesystem
#   ${PROJECT_SOURCE_DIR}/src/flatbuffers/wal_generated.fb.h)
target_link_libraries (smf_filesystem
  ${SEASTAR_LDFLAGS}
  smftracing
  boost_regex
  xxhash_static
  zstd.a
  re2
  )
