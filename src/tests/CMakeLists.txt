set(TOOR ${PROJECT_SOURCE_DIR}/src/tests)
smf_test(
  UNIT_TEST
  BINARY_NAME randomstr
  SOURCES ${TOOR}/randomstr.cc
  SOURCE_DIRECTORY ${TOOR}
  LIBRARIES smf Googletest::gtest
  )
smf_test(
  UNIT_TEST
  BINARY_NAME simple_hist
  SOURCES ${TOOR}/histogram_tests.cc
  SOURCE_DIRECTORY ${TOOR}
  LIBRARIES smf Googletest::gtest
  )
