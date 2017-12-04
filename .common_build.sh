#!/bin/bash
set -e
git_root=$(git rev-parse --show-toplevel)
PATH=$git_root/src/third_party/bin:$PATH
nprocs=$(grep -c ^processor /proc/cpuinfo)

ninja_cmd='ninja-build -v'

os=$(lsb_release 2> /dev/null -si | tr '[:upper:]' '[:lower:]')
if [[ ${os} == "ubuntu" ]]; then
    ninja_cmd='ninja -v'
fi

CC=${git_root}/src/third_party/bin/gcc
CXX=${git_root}/src/third_party/bin/g++

cmake_cmd=${git_root}/src/third_party/bin/cmake
ctest_cmd=${git_root}/src/third_party/bin/cmake


# TODO(agallego) - I have a bug in the cmake I need to fix
export LD_LIBRARY_PATH=${git_root}/src/third_party/lib:${git_root}/src/third_party/lib64:$LD_LIBRARY_PATH
