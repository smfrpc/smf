#!/bin/bash

# Copyright 2019 SMF Authors
#

set -e
set -x

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
proj_dir=$(realpath "${this_dir}"/../)

# shellcheck disable=SC1091
source /etc/os-release

CMAKE="cmake"
CTEST="ctest"
case ${ID} in
  centos|rhel)
    MAJOR_VERSION="$(echo "$VERSION_ID" | cut -d. -f1)"
    if test "$MAJOR_VERSION" = 7 ; then
      CMAKE="cmake3"
      CTEST="ctest3"
      # shellcheck disable=SC1091
      source /opt/rh/devtoolset-8/enable
    fi
esac

: "${BUILD_GENERATOR:=Unix Makefiles}"
build_cmd="make VERBOSE=1"
if [ "${BUILD_GENERATOR}" = "Ninja" ]; then
  build_cmd="ninja -v"
fi

# setup temp dirs
build_dir=$(mktemp -d)
trap 'rm -rf ${build_dir}' EXIT

(
  cd "${build_dir}"
  ${CMAKE} -G"${BUILD_GENERATOR}" -DSMF_MANAGE_DEPS=ON "${proj_dir}"
  ${build_cmd} -j"$(nproc)"
  ls -l bin
  ldd bin/smf_demo_server | grep smf-deps-install
  ldd bin/smf_demo_server | grep -v smf-deps-install
  ${CTEST} -V
)
