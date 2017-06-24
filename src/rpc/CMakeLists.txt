set(RPC_LIB_SOURCES
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_handle_router.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_envelope.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_server.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_recv_context.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_client.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_client_stats.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_recv_context.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_connection_limits.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_utils.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_letter.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/filters/zstd_filter.cc
  )
add_library(smf_rpc STATIC ${RPC_LIB_SOURCES})
target_link_libraries(smf_rpc smf_utils smf_tracing)
add_dependencies(smf_rpc rpc_serialization)
install(TARGETS smf_rpc DESTINATION lib)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/rpc/smf_gen)
