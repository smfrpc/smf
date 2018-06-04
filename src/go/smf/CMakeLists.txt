go_build_lib(
  TARGET_NAME gosmf
  FOLDER ${CMAKE_CURRENT_SOURCE_DIR}
  GO_GET_TARGETS
    "github.com/cespare/xxhash"
    "github.com/google/flatbuffers/go"
    "github.com/stretchr/testify/assert"
  )
