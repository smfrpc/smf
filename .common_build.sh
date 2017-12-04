#!/bin/bash

# Copyright 2017 Alexander Gallego
#

set -e
git_root=$(git rev-parse --show-toplevel)
PATH=$git_root/src/third_party/bin:$PATH
nprocs=$(grep -c ^processor /proc/cpuinfo)

ninja_cmd='ninja-build -v'

os=$(lsb_release 2> /dev/null -si | tr '[:upper:]' '[:lower:]')
if [[ ${os} == "ubuntu" ]]; then
    ninja_cmd='ninja -v'
fi

cmake_cmd=${git_root}/src/third_party/bin/cmake
ctest_cmd=${git_root}/src/third_party/bin/ctest
