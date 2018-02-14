set(TEST_ROOT ${PROJECT_SOURCE_DIR}/src/test)

smf_test(
  UNIT_TEST
  BINARY_NAME wal_epoch
  SOURCES ${TEST_ROOT}/wal_epoch_extractor/wal_epoch_extractor_test.cc
  SOURCE_DIRECTORY ${TEST_ROOT}/wal_epoch_extractor
  LIBRARIES gtest smf_filesystem
  )
smf_test(
  UNIT_TEST
  BINARY_NAME file_size_utils
  SOURCES ${TEST_ROOT}/file_size_utils/main.cc
  SOURCE_DIRECTORY ${TEST_ROOT}/file_size_utils
  LIBRARIES gtest smf_filesystem
  )
smf_test(
  UNIT_TEST
  BINARY_NAME human_output
  SOURCES ${TEST_ROOT}/human_bytes_printing_utils/main.cc
  SOURCE_DIRECTORY ${TEST_ROOT}/human_bytes_printing_utils
  LIBRARIES gtest smf_filesystem
  )
smf_test(
  UNIT_TEST
  BINARY_NAME wal_functor
  SOURCES ${TEST_ROOT}/wal_head_file_functor_tests/main.cc
  SOURCE_DIRECTORY ${TEST_ROOT}/wal_head_file_functor_tests
  LIBRARIES gtest smf_filesystem
  )
smf_test(
  UNIT_TEST
  BINARY_NAME clockpro
  SOURCES ${TEST_ROOT}/clock_pro/main.cc
  SOURCE_DIRECTORY ${TEST_ROOT}/clock_pro
  LIBRARIES gtest smf_filesystem
  )
smf_test(
  UNIT_TEST
  BINARY_NAME compressors
  SOURCES ${TEST_ROOT}/compressor/main.cc
  SOURCE_DIRECTORY ${TEST_ROOT}/compressor
  LIBRARIES gtest smf_utils
  )
smf_test(
  UNIT_TEST
  BINARY_NAME fbs_size
  SOURCES ${TEST_ROOT}/flatbuffers/main.cc
  SOURCE_DIRECTORY ${TEST_ROOT}/flatbuffers
  LIBRARIES flatbuffers
  )
