add_custom_command (
  OUTPUT ${PROJECT_SOURCE_DIR}/src/flatbuffers/raft.smf.fb.h
  COMMAND smf_gen --logtostderr --filename ${PROJECT_SOURCE_DIR}/src/flatbuffers/raft.fbs
  DEPENDS smf_gen
  )

add_custom_target(__raft_smfgen ALL DEPENDS ${PROJECT_SOURCE_DIR}/src/flatbuffers/raft.smf.fb.h)
add_executable (raft
  ${PROJECT_SOURCE_DIR}/src/raft/main.cc)
target_link_libraries (raft smfrpc)
add_dependencies(raft __raft_smfgen)
