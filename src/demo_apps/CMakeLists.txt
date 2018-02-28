smf_generate_cpp(demo_fbs demo_service.fbs)

add_executable(demo_server server/main.cc ${demo_fbs_OUTPUTS})
target_link_libraries(demo_server smf)

add_executable(demo_client client/main.cc ${demo_fbs_OUTPUTS})
target_link_libraries(demo_client smf)

install(TARGETS
  demo_server
  demo_client
  DESTINATION bin)
