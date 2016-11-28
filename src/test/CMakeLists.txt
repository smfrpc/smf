set(TEST_ROOT ${PROJECT_SOURCE_DIR}/src/test)
set(TEST_RUNNER ${PROJECT_SOURCE_DIR}/src/test_runner.py)
set(FAST_TESTS "")
set(SLOW_TESTS "")

# You need the name and the directory is going to execute.
# The reason is that you ned to copy all of the products of the
# directory in case it has some static file that it reads for
# verification of output, etc.
#
function (register_fast_test NAME DIR)
  set (FAST_TESTS
    ${FAST_TESTS}
    ${NAME} PARENT_SCOPE)
  add_test (
    NAME ${NAME}_runner
    COMMAND "${TEST_RUNNER}" --test_type unit --binary $<TARGET_FILE:${NAME}> --directory ${DIR}
    #WORKING_DIRECTORY ${DIR}
    )
endfunction ()


set(ALL_TESTS "")
IF(CMAKE_ALL_TESTS)
  set(ALL_TESTS ${FAST_TESTS} ${SLOW_TESTS})
ELSE()
  set(ALL_TESTS ${FAST_TESTS})
ENDIF()


add_executable (simple_flatbuffers_tests ${TEST_ROOT}/flatbuffers/simple.cc)
target_link_libraries(simple_flatbuffers_tests -lgtest smfrpc)
register_fast_test (simple_flatbuffers_tests ${TEST_ROOT}/flatbuffers)


add_executable(wal_name_re ${TEST_ROOT}/wal_name_parser_tests/wal_name_re.cc)
target_link_libraries(wal_name_re -lgtest smf_filesystem)
register_fast_test (wal_name_re ${TEST_ROOT}/wal_name_parser_tests)

# TODO(agallego) - add slow tests too if a flag is passed
# -DALL_TESTS will enable slow and fast tests
# define a new variable called set(ALL_TESTS) and add both
# slow and fast tests there
#
add_custom_target (check
  COMMAND ctest --output-on-failure
  DEPENDS ${ALL_TESTS})
