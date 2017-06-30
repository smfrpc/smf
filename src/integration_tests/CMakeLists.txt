set(IT_ROOT ${PROJECT_SOURCE_DIR}/src/integration_tests)
set(TEST_RUNNER ${PROJECT_SOURCE_DIR}/src/test_runner.py)
set(IT_TESTS "")
function (register_integration_test NAME DIR)
  set(IT_TESTS
    ${IT_TESTS}
    ${NAME} PARENT_SCOPE)
  add_test (
    NAME ${NAME}_integration_test
    COMMAND ${TEST_RUNNER} --type integration --binary $<TARGET_FILE:${NAME}> --directory ${DIR}
    )
endfunction ()


add_executable(wal_writer ${IT_ROOT}/wal_writer/main.cc)
target_link_libraries(wal_writer smf_filesystem)
register_integration_test(wal_writer ${IT_ROOT}/wal_writer)

add_executable(histograms ${IT_ROOT}/histograms/main.cc)
target_link_libraries(histograms smf_filesystem)
register_integration_test(histograms ${IT_ROOT}/histograms)

## Multiple tests depend on this custom command

add_custom_command (
  OUTPUT ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.smf.fb.h
  COMMAND smf_gen --logtostderr --filename ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.fbs
  DEPENDS smf_gen
  DEPENDS ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.fbs
  )
# BOTH tests depend on the custom_command
add_executable(rpc
  ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.smf.fb.h
  ${IT_ROOT}/rpc/main.cc)
target_link_libraries(rpc smf_rpc)
register_integration_test(rpc ${IT_ROOT}/rpc)
add_executable(rpc_recv_timeout
  ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.smf.fb.h
  ${IT_ROOT}/rpc_recv_timeout/main.cc)
target_link_libraries(rpc_recv_timeout smf_rpc)
register_integration_test(rpc_recv_timeout ${IT_ROOT}/rpc_recv_timeout)



add_executable(wal ${IT_ROOT}/wal/main.cc)
target_link_libraries(wal smf_filesystem smf_rpc)
register_integration_test(wal ${IT_ROOT}/wal)

add_executable(clock_pro ${IT_ROOT}/wal_clock_pro_cache/main.cc)
target_link_libraries(clock_pro smf_filesystem)
register_integration_test(clock_pro ${IT_ROOT}/wal_writer)


add_executable(chain_replication_utils
  ${IT_ROOT}/chain_replication_utils/main.cc)
target_link_libraries(chain_replication_utils smf_cr flatbuffers)
register_fast_test (chain_replication_utils
  ${IT_ROOT}/chain_replication_utils)



add_custom_target(integration
  COMMAND ctest --output-on-failure
  DEPENDS ${IT_TESTS})
