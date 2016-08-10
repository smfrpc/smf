add_custom_command (
  OUTPUT ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/demo_service.smf.fb.h
  COMMAND smf_gen --logtostderr --filename ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/demo_service.fbs
  DEPENDS smf_gen
  )

# template server
add_executable (rpc_template_server
  ${PROJECT_SOURCE_DIR}/src/rpc/templates/server.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/demo_service.smf.fb.h)
target_link_libraries (rpc_template_server smfrpc)

# template client
add_executable (rpc_template_client
  ${PROJECT_SOURCE_DIR}/src/rpc/templates/client.cc
  ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/demo_service.smf.fb.h)
target_link_libraries (rpc_template_client smfrpc)
