add_executable (smfb
  ${PROJECT_SOURCE_DIR}/src/smfb/main.cc
  ${PROJECT_SOURCE_DIR}/src/smfb/smfb_command_line_options.cc
  )
target_link_libraries (smfb
  ${SEASTAR_DPDK_LIBS}
  smf_rpc smf_raft smf_cr smf_filesystem)
install(TARGETS smfb DESTINATION bin)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/smfb/client)
