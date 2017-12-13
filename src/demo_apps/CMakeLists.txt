add_executable(demo_server
  server/main.cc)
target_link_libraries(demo_server
  smf_rpc
  demo_service)

add_executable(demo_client
  client/main.cc)
target_link_libraries(demo_client
  smf_rpc
  demo_service)

install(TARGETS
  demo_server
  demo_client
  DESTINATION bin)
