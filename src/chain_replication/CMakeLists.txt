add_library(smf_cr STATIC
  chain_replication_service.cc)

target_link_libraries(smf_cr
  smf_rpc
  cr_service)

install(TARGETS smf_cr DESTINATION lib)
