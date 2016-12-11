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

add_executable(rpc ${IT_ROOT}/rpc/main.cc)
target_link_libraries(rpc smfrpc)
register_integration_test(rpc ${IT_ROOT}/rpc)


add_custom_target(integration
  COMMAND ctest --output-on-failure
  DEPENDS ${IT_TESTS})
