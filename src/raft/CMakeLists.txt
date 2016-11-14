add_custom_command (
  OUTPUT ${PROJECT_SOURCE_DIR}/src/flatbuffers/raft.smf.fb.h
  COMMAND smf_gen --logtostderr --filename ${PROJECT_SOURCE_DIR}/src/flatbuffers/raft.fbs
  DEPENDS smf_gen
  DEPENDS ${PROJECT_SOURCE_DIR}/src/flatbuffers/raft.fbs
  )
add_library (raft STATIC
  ${PROJECT_SOURCE_DIR}/src/flatbuffers/raft.smf.fb.h)
target_link_libraries (raft smfrpc)
