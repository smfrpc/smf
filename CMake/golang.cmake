find_program(GO go)
if (NOT GO)
  message(FATAL_ERROR "Cannot find `go` binary. Disable go via -DSMF_ENABLE_GO=OFF")
endif()


set(GOPATH "${CMAKE_BINARY_DIR}")
file(MAKE_DIRECTORY
  "${GOPATH}/src"
  "${GOPATH}/pkg/linux_amd64/gosmf"
  "${GOPATH}/src/github.com/senior7515"
  )
set(symlink_name "godep_symlink_${GOPATH}")
string(REGEX REPLACE "/" "_" symlink_name ${symlink_name})

# make the build directory compatible w/ go build ./...
add_custom_target("${symlink_name}" ALL
  COMMAND ${CMAKE_COMMAND}
  -E create_symlink "${PROJECT_SOURCE_DIR}" "${GOPATH}/src/github.com/senior7515/smf")

set(ENV{GOPATH} ${GOPATH})
