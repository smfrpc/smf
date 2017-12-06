set(RPC_LIB_SOURCES
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_handle_router.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_envelope.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_server.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_recv_context.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_client.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_client_stats.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_recv_context.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_connection_limits.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_letter.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/filters/zstd_filter.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/filters/lz4_filter.cc
  )
add_library(smf_rpc STATIC ${RPC_LIB_SOURCES})
target_link_libraries(smf_rpc smf_utils smf_tracing)
add_dependencies(smf_rpc rpc_serialization)
install(TARGETS smf_rpc DESTINATION lib)
install(FILES
  ${PROJECT_SOURCE_DIR}/src/rpc/futurize_utils.h
  ${PROJECT_SOURCE_DIR}/src/rpc/load_gen/generator_args.h
  ${PROJECT_SOURCE_DIR}/src/rpc/load_gen/generator_duration.h
  ${PROJECT_SOURCE_DIR}/src/rpc/load_gen/load_channel.h
  ${PROJECT_SOURCE_DIR}/src/rpc/load_gen/load_generator.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_client.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_client_stats.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_connection.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_connection_limits.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_envelope.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_filter.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_handle_router.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_handle_router.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_header_utils.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_letter.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_recv_context.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_recv_typed_context.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_server.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_server_connection.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_server_stats.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_service.h
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_typed_envelope.h
  DESTINATION include/smf/rpc
  )
install(FILES
  ${PROJECT_SOURCE_DIR}/src/rpc/filters/lz4_filter.h
  ${PROJECT_SOURCE_DIR}/src/rpc/filters/zstd_filter.h
  DESTINATION include/smf/rpc/filters
  )
add_subdirectory(${PROJECT_SOURCE_DIR}/src/rpc/smf_gen)
