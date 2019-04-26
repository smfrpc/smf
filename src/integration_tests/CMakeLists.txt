smfc_gen(
  CPP
  TARGET_NAME demo_test_fbs
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES ${PROJECT_SOURCE_DIR}/demo_apps/demo_service.fbs)

set(IT_ROOT ${PROJECT_SOURCE_DIR}/src/integration_tests)

smf_test(
  INTEGRATION_TEST
  BINARY_NAME histograms
  SOURCES ${IT_ROOT}/histograms/main.cc
  SOURCE_DIRECTORY ${IT_ROOT}/histograms
  INCLUDES ${PROJECT_SOURCE_DIR}/src
  INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/..
  LIBRARIES smf
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME rpc
  SOURCES ${IT_ROOT}/rpc/main.cc ${demo_test_fbs}
  SOURCE_DIRECTORY ${IT_ROOT}/rpc
  INCLUDES ${PROJECT_SOURCE_DIR}/src
  INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/..
  LIBRARIES smf
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME rpc_recv_timeout
  SOURCES ${IT_ROOT}/rpc_recv_timeout/main.cc ${demo_test_fbs}
  SOURCE_DIRECTORY ${IT_ROOT}/rpc_recv_timeout
  INCLUDES ${PROJECT_SOURCE_DIR}/src
  INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/..
  LIBRARIES smf
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME rpc_multiple_remote_addrs
  SOURCES ${IT_ROOT}/rpc_multiple_remote_addrs/main.cc ${demo_test_fbs}
  SOURCE_DIRECTORY ${IT_ROOT}/rpc_multiple_remote_addrs
  INCLUDES ${PROJECT_SOURCE_DIR}/src
  INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/..
  LIBRARIES smf
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME rpc_backpressure
  SOURCES ${IT_ROOT}/rpc_backpressure/main.cc ${demo_test_fbs}
  SOURCE_DIRECTORY ${IT_ROOT}/rpc_backpressure
  INCLUDES ${PROJECT_SOURCE_DIR}/src
  INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/..
  LIBRARIES smf
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME rpc_send_client_timeout
  SOURCES ${IT_ROOT}/rpc_send_timeout/main.cc ${demo_test_fbs}
  SOURCE_DIRECTORY ${IT_ROOT}/rpc_send_timeout
  INCLUDES ${PROJECT_SOURCE_DIR}/src
  INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/..
  LIBRARIES smf
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME rpc_reconnect_with_timeout
  SOURCES ${IT_ROOT}/rpc_reconnect_with_timeout/main.cc ${demo_test_fbs}
  SOURCE_DIRECTORY ${IT_ROOT}/rpc_reconnect_with_timeout
  INCLUDES ${PROJECT_SOURCE_DIR}/src
  INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/..
  LIBRARIES smf
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME rpc_hystrix_basic
  SOURCES ${IT_ROOT}/hystrix/main.cc ${demo_test_fbs}
  SOURCE_DIRECTORY ${IT_ROOT}/hystrix
  INCLUDES ${PROJECT_SOURCE_DIR}/src
  INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/..
  LIBRARIES smf
  )


add_subdirectory(rpc_bad_msg_t)
