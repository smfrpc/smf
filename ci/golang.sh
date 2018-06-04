#!/bin/bash
set -evx
git_root=$(git rev-parse --show-toplevel)
export GOPATH=${git_root}/build
fake_root=${GOPATH}/src/smf
cd ${git_root}
if [[ ! -e ${GOPATH}/src/smf ]]; then
    mkdir -p $GOPATH
    go get -u github.com/golang/dep/cmd/dep
    mkdir -p ${GOPATH}/src
    ln -s ${git_root} ${fake_root}
    cd ${fake_root}
    dep ensure
fi
export PATH=$GOPATH/bin:$PATH
cd ${fake_root}/src/go/smf
mkdir -p ${GOPATH}/pkg/linux_amd64/smf
go test ./...
go build -o ${GOPATH}/pkg/linux_amd64/smf/smf.a .
