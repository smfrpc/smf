#!/bin/bash
set -e
git_root=$(git rev-parse --show-toplevel)
export PATH=$git_root/src/third_party/bin:$PATH
nprocs=$(grep -c ^processor /proc/cpuinfo)

ninja_cmd='ninja-build -v'

os=$(lsb_release 2> /dev/null -si | tr '[:upper:]' '[:lower:]')
if [[ ${os} == "ubuntu" ]]; then
    ninja_cmd='ninja -v'
fi
