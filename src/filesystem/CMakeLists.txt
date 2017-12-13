add_library(smf_filesystem STATIC
  wal_writer_node.cc
  wal_writer.cc
  write_ahead_log.cc
  wal_clock_pro_cache.cc
  wal_reader.cc
  wal_reader_node.cc
  wal_impl_with_cache.cc
  file_size_utils.cc
  wal_name_extractor_utils.cc
  wal_name_extractor_utils.cc
  wal_writer_utils.cc
  wal_pretty_print_utils.cc
  wal_opts.cc
  wal_writer_file_lease.cc
  wal_write_projection.cc
  wal_partition_manager.cc
  wal_write_behind_cache.cc
  wal_lcore_map.cc)

target_link_libraries(smf_filesystem
  smf_utils
  re2)

# TODO: remove this when code gen uses better library interface
add_dependencies(smf_filesystem rpc_serialization)

install(TARGETS smf_filesystem DESTINATION lib)
