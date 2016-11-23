set(IT_ROOT ${PROJECT_SOURCE_DIR}/src/integration_tests)
set(IT_TESTS "")
function (register_test NAME DIR)
  set(IT_TESTS
    ${IT_TESTS}
    ${NAME} PARENT_SCOPE)
  add_test (
    NAME ${NAME}_integration_test
    COMMAND ${IT_ROOT}/it_test.py --binary $<TARGET_FILE:${NAME}> --directory ${DIR}
    WORKING_DIRECTORY ${DIR}
    )
endfunction ()


add_executable(wal_writer ${IT_ROOT}/wal_writer/main.cc)
target_link_libraries(wal_writer smf_filesystem)
register_fast_test(wal_writer ${IT_ROOT}/wal_writer)

add_custom_target(integration
  COMMAND ctest --output-on-failure
  DEPENDS ${IT_TESTS})
