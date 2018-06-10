find_program(GO go)
if (NOT GO)
  message(FATAL_ERROR "Cannot find `go` binary. Disable go via -DSMF_ENABLE_GO=OFF")
endif()


set(GOPATH "${CMAKE_BINARY_DIR}")
set(ENV{GOPATH} ${GOPATH})
