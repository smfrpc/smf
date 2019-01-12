set(IT_ROOT ${PROJECT_SOURCE_DIR}/src/integration_tests)
smfc_gen(
  CPP
  TARGET_NAME bad_msg_fbs
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/bad_msg.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/bad_svc.fbs)
smf_test(
  INTEGRATION_TEST
  BINARY_NAME rpc_bad_msg_t
  SOURCES main.cc ${bad_msg_fbs}
  SOURCE_DIRECTORY ${IT_ROOT}/rpc_bad_msg_t
  INCLUDES ${CMAKE_CURRENT_BINARY_DIR}
  INCLUDES ${PROJECT_SOURCE_DIR}/src
  LIBRARIES smf
  )
