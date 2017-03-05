set(RPC_LIB_SOURCES
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_handle_router.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_server_stats_printer.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_envelope.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_server_stats.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_server.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_recv_context.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_client.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_client_stats.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_recv_context.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_connection_limits.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/rpc_utils.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/filters/zstd_filter.cc
  )
add_library(smfrpc STATIC ${RPC_LIB_SOURCES})
target_link_libraries(smfrpc
  ${SEASTAR_LDFLAGS}
  smftracing
  xxhash_static
  zstd.a
  )
add_dependencies(smfrpc rpc_serialization)
install(TARGETS smfrpc DESTINATION lib)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/rpc/smf_gen)
