smf_generate_cpp(demo_fbs demo_service.fbs)

set(IT_ROOT ${PROJECT_SOURCE_DIR}/src/integration_tests)

smf_test(
  INTEGRATION_TEST
  BINARY_NAME histograms
  SOURCES ${IT_ROOT}/histograms/main.cc
  SOURCE_DIRECTORY ${IT_ROOT}/histograms
  LIBRARIES smf
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME rpc
  SOURCES ${IT_ROOT}/rpc/main.cc ${demo_fbs_OUTPUTS}
  SOURCE_DIRECTORY ${IT_ROOT}/rpc
  LIBRARIES smf
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME rpc_recv_timeout
  SOURCES ${IT_ROOT}/rpc_recv_timeout/main.cc ${demo_fbs_OUTPUTS}
  SOURCE_DIRECTORY ${IT_ROOT}/rpc_recv_timeout
  LIBRARIES smf
  )
