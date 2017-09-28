set(IT_ROOT ${PROJECT_SOURCE_DIR}/src/integration_tests)

## Multiple tests depend on this custom command
add_custom_command (
  OUTPUT ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.smf.fb.h
  COMMAND smf_gen --logtostderr --filename ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.fbs
  DEPENDS smf_gen
  DEPENDS ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.fbs
  )

smf_test(
  INTEGRATION_TEST
  BINARY_NAME wal_writer
  SOURCES ${IT_ROOT}/wal_writer/main.cc
  SOURCE_DIRECTORY ${IT_ROOT}/wal_writer
  LIBRARIES smf_filesystem
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME histograms
  SOURCES ${IT_ROOT}/histograms/main.cc
  SOURCE_DIRECTORY ${IT_ROOT}/histograms
  LIBRARIES smf_filesystem
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME rpc
  SOURCES ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.smf.fb.h ${IT_ROOT}/rpc/main.cc
  SOURCE_DIRECTORY ${IT_ROOT}/rpc
  LIBRARIES smf_rpc
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME rpc_recv_timeout
  SOURCES ${PROJECT_SOURCE_DIR}/src/flatbuffers/demo_service.smf.fb.h ${IT_ROOT}/rpc_recv_timeout/main.cc
  SOURCE_DIRECTORY ${IT_ROOT}/rpc_recv_timeout
  LIBRARIES smf_rpc
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME wal
  SOURCES ${IT_ROOT}/wal/main.cc
  SOURCE_DIRECTORY ${IT_ROOT}/wal
  LIBRARIES smf_filesystem smf_rpc smf_cr
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME clock_pro
  SOURCES ${IT_ROOT}/wal_clock_pro_cache/main.cc
  SOURCE_DIRECTORY ${IT_ROOT}/wal_clock_pro_cache
  LIBRARIES smf_filesystem
  )
smf_test(
  INTEGRATION_TEST
  BINARY_NAME cr
  SOURCES ${IT_ROOT}/chain_replication_utils/main.cc
  SOURCE_DIRECTORY ${IT_ROOT}/chain_replication_utils
  LIBRARIES smf_cr flatbuffers
  )

