add_executable (smfb
  main.cc
  smfb_command_line_options.cc)

target_link_libraries (smfb
  ${SEASTAR_DPDK_LIBS}
  smf_rpc smf_cr smf_filesystem)
install(TARGETS smfb DESTINATION bin)
add_subdirectory(client)
