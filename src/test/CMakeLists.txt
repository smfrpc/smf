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

add_executable(wal_epoch_extractor
  ${TEST_ROOT}/wal_epoch_extractor/wal_epoch_extractor_test.cc)
target_link_libraries(wal_epoch_extractor -lgtest smf_filesystem)
register_fast_test (wal_epoch_extractor ${TEST_ROOT}/wal_epoch_extractor)


add_executable(file_size_utils ${TEST_ROOT}/file_size_utils/main.cc)
target_link_libraries(file_size_utils -lgtest)
register_fast_test (file_size_utils ${TEST_ROOT}/file_size_utils)

add_executable(human_bytes_printing_utils
  ${TEST_ROOT}/human_bytes_printing_utils/main.cc)
target_link_libraries(human_bytes_printing_utils -lgtest)
register_fast_test (human_bytes_printing_utils
  ${TEST_ROOT}/human_bytes_printing_utils)

add_executable(wal_head_file_functor_tests
  ${TEST_ROOT}/wal_head_file_functor_tests/main.cc)
target_link_libraries(wal_head_file_functor_tests -lgtest smf_filesystem)
register_fast_test (wal_head_file_functor_tests
  ${TEST_ROOT}/wal_head_file_functor_tests)

add_custom_target (check
  COMMAND ctest --output-on-failure
  DEPENDS ${ALL_TESTS})
