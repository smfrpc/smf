add_subdirectory(smf_gen)

install(FILES
  load_gen/generator_args.h
  load_gen/generator_duration.h
  load_gen/load_channel.h
  load_gen/load_generator.h
  DESTINATION include/smf/rpc/load_gen)

install(FILES
  futurize_utils.h
  rpc_client.h
  rpc_client_stats.h
  rpc_connection.h
  rpc_connection_limits.h
  rpc_envelope.h
  rpc_filter.h
  rpc_handle_router.cc
  rpc_handle_router.h
  rpc_header_utils.h
  rpc_letter.h
  rpc_recv_context.h
  rpc_recv_typed_context.h
  rpc_server.h
  rpc_server_connection.h
  rpc_server_stats.h
  rpc_service.h
  rpc_typed_envelope.h
  DESTINATION include/smf/rpc
  )
install(FILES
  filters/lz4_filter.h
  filters/zstd_filter.h
  DESTINATION include/smf/rpc/filters
  )
