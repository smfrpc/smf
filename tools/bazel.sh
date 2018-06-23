#!/bin/bash

# Copyright 2018 SMF Authors
#

set -e
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++


function usage {
    cat <<EOM
Usage: $(basename "$0") [OPTION]...
  -a         build all
EOM
    exit 1
}

function build_all {
    bazel build --verbose_failures --sandbox_debug //src/smfc/...
    bazel build --verbose_failures --sandbox_debug //src/go/...
    bazel test --verbose_failures --sandbox_debug //src/go/...
}

while getopts ":a" optKey; do
    case $optKey in
        a)
            build_all
            ;;
        *)
            exec bazel "$@"
            ;;
    esac
done
