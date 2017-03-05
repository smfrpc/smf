add_custom_command (
  OUTPUT ${PROJECT_SOURCE_DIR}/src/flatbuffers/chain_replication.smf.fb.h
  COMMAND smf_gen --logtostderr --filename ${PROJECT_SOURCE_DIR}/src/flatbuffers/chain_replication.fbs
  DEPENDS smf_gen
  DEPENDS ${PROJECT_SOURCE_DIR}/src/flatbuffers/chain_replication.fbs
  )

add_library (cr STATIC
  ${PROJECT_SOURCE_DIR}/src/flatbuffers/chain_replication.smf.fb.h
  ${PROJECT_SOURCE_DIR}/src/chain_replication/chain_replication_service.cc
  ${PROJECT_SOURCE_DIR}/src/chain_replication/chain_replication_utils.cc
  )
# add_dependencies(cr
#   ${PROJECT_SOURCE_DIR}/src/flatbuffers/chain_replication.smf.fb.h)
target_link_libraries (cr smfrpc)
install(TARGETS cr DESTINATION lib)
