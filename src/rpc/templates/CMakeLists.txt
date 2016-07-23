add_custom_command (
  OUTPUT ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/demo_service.smf.fb.h
  COMMAND smf_gen --logtostderr --filename ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/demo_service.fbs
  DEPENDS smf_gen
  )
add_custom_target(__rpc_demo_smfgen ALL DEPENDS ${PROJECT_SOURCE_DIR}/src/rpc/smf_gen/demo_service.smf.fb.h)

# template server
add_executable (rpc_template_server
  ${PROJECT_SOURCE_DIR}/src/rpc/templates/server.cc)
target_link_libraries (rpc_template_server smfrpc)
add_dependencies(rpc_template_server __rpc_demo_smfgen)

# template client
add_executable (rpc_template_client
  ${PROJECT_SOURCE_DIR}/src/rpc/templates/client.cc)
target_link_libraries (rpc_template_client smfrpc)
add_dependencies(rpc_template_client __rpc_demo_smfgen)
