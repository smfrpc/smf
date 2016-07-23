# template server
add_executable (rpc_template_server
  ${PROJECT_SOURCE_DIR}/src/rpc/templates/server.cc)
target_link_libraries (rpc_template_server smfrpc)


# template client
add_executable (rpc_template_client
  ${PROJECT_SOURCE_DIR}/src/rpc/templates/client.cc)
target_link_libraries (rpc_template_client smfrpc)
