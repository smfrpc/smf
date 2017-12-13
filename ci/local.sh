#!/bin/bash

set -e
set -x

THIS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

IMAGES="
fedora:27
fedora:26
fedora:25
ubuntu:zesty
ubuntu:artful
debian:stretch
debian:sid
debian:buster
"

for img in ${IMAGES}; do
  if [[ ${img} != -* ]]; then
    DOCKER_IMAGE=${img} USE_NINJA=  ${THIS_DIR}/script.sh
    DOCKER_IMAGE=${img} USE_NINJA=1 ${THIS_DIR}/script.sh
    # clang issues in seastar...
    #DOCKER_IMAGE=${img} USE_CLANG=1 ${THIS_DIR}/ci/script.sh
  fi
done
