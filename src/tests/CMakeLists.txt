find_package (GTest REQUIRED CONFIG)

set(TOOR ${PROJECT_SOURCE_DIR}/src/tests)
smf_test(
  UNIT_TEST
  BINARY_NAME randomstr
  SOURCES ${TOOR}/randomstr.cc
  SOURCE_DIRECTORY ${TOOR}
  LIBRARIES smf GTest::gtest
  )
smf_test(
  UNIT_TEST
  BINARY_NAME simple_hist
  SOURCES ${TOOR}/histogram_tests.cc
  SOURCE_DIRECTORY ${TOOR}
  LIBRARIES smf GTest::gtest
  )

add_executable(smf_histgen histgen.cc)
target_link_libraries(smf_histgen smf)
target_include_directories(smf_histgen
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${PROJECT_SOURCE_DIR}/src/include
)
