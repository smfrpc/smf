set(actual_source_directory
  "${GOPATH}/src/github.com/senior7515/smf/src/go/smf")
add_custom_target(gosmf)
add_custom_command(
  TARGET gosmf
  COMMAND export GOPATH=${GOPATH} && cd ${GOPATH} && ${GO} get github.com/golang/dep/cmd/dep
  COMMAND export GOPATH=${GOPATH} && cd ${actual_source_directory}/.. && ${GOPATH}/bin/dep ensure
  COMMAND export GOPATH=${GOPATH} && cd ${actual_source_directory} && ${GO} build -o ${GOPATH}/pkg/linux_amd64/gosmf/gosmf.a ./...
  )

add_custom_target(gosmf_all ALL DEPENDS gosmf)
