add_executable (demo_server
  ${PROJECT_SOURCE_DIR}/src/demo_apps/server/main.cc
  )
add_executable (demo_client
  ${PROJECT_SOURCE_DIR}/src/demo_apps/client/main.cc
  )
target_link_libraries (demo_server smf_rpc)
target_link_libraries (demo_client smf_rpc)

install(TARGETS demo_server DESTINATION bin)
install(TARGETS demo_client DESTINATION bin)
