#!/bin/bash --login
set -ex
lsb_release -a
git_root=$(git rev-parse --show-toplevel)
nprocs=$(grep -c ^processor /proc/cpuinfo)

mkdir -p $git_root/build
cd $git_root/build
cmake $git_root
make -j$nprocs
ctest -j$nprocs -V
