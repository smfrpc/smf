smfc_gen(
  GOLANG
  TARGET_NAME go_api
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES ${PROJECT_SOURCE_DIR}/src/core/rpc.fbs)

go_build(
  TARGET_NAME gosmf
  FOLDER ${CMAKE_CURRENT_SOURCE_DIR}
  GO_GET_TARGETS
    "github.com/cespare/xxhash"
    "github.com/google/flatbuffers/go"
    "github.com/stretchr/testify/assert"
  )
