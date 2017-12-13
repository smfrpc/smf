add_executable (smfb_low_level_client
  low_level_client.cc)

target_link_libraries (smfb_low_level_client
  ${SEASTAR_DPDK_LIBS} smf_rpc smf_cr)
install(TARGETS smfb_low_level_client RUNTIME DESTINATION bin)
