#!/bin/bash
set -e
set -x

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
proj_dir=${this_dir}/../

# setup temp dirs
build_dir=$(mktemp -d)
trap "rm -rf ${build_dir}" EXIT

pushd ${build_dir}
cmake ${proj_dir}
make -j$(nproc) VERBOSE=1
ls -l bin
popd
