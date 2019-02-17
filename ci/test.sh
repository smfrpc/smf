#!/bin/bash
set -e
set -x

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
proj_dir=$(realpath ${this_dir}/../)

. /etc/os-release

CMAKE="cmake"
case ${ID} in
  centos|rhel)
    MAJOR_VERSION="$(echo $VERSION_ID | cut -d. -f1)"
    if test $MAJOR_VERSION = 7 ; then
      CMAKE="cmake3"
      source /opt/rh/devtoolset-8/enable
    fi
esac

# setup temp dirs
build_dir=$(mktemp -d)
trap "rm -rf ${build_dir}" EXIT

(
  cd ${build_dir}
  ${CMAKE} ${proj_dir}
  make -j$(nproc) VERBOSE=1
  ls -l bin
)
