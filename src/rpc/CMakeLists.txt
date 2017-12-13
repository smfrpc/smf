set(RPC_LIB_SOURCES
  rpc_handle_router.cc
  rpc_envelope.cc
  rpc_server.cc
  rpc_recv_context.cc
  rpc_client.cc
  rpc_client_stats.cc
  rpc_recv_context.cc
  rpc_connection_limits.cc
  rpc_letter.cc
  filters/zstd_filter.cc
  filters/lz4_filter.cc)
add_library(smf_rpc STATIC ${RPC_LIB_SOURCES})
target_link_libraries(smf_rpc smf_utils smf_tracing)
add_dependencies(smf_rpc rpc_serialization)
install(TARGETS smf_rpc DESTINATION lib)

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

add_subdirectory(smf_gen)
