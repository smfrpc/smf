#!/bin/bash

# Copyright 2018 SMF Authors
#


set -e

. /etc/os-release

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="${this_dir}/../"
buildcmd="ninja"
buildtype="debug"
builddir=$root/build/debug
case $ID in
    debian|ubuntu|linuxmint)
        echo "$ID supported"
        buildcmd="ninja"
        ;;
    centos|fedora)
        echo "$ID supported"
        buildcmd="ninja-build"
        ;;
    *)
        echo "$ID not supported"
        exit 1
        ;;
esac

function debug {
    echo "Debug"
    builddir=$root/build/debug
    mkdir -p $builddir
    cd ${builddir}
    cmake -Wdev \
          --debug-output \
          -DCMAKE_VERBOSE_MAKEFILE=ON -G Ninja \
          -DCMAKE_INSTALL_PREFIX=${builddir} \
          -DSMF_ENABLE_CMAKE_PROJECT_FLAGS=ON \
          -DCMAKE_BUILD_TYPE=Debug ${root}

    # for fmt.py
    ln -sfn "${builddir}/compile_commands.json" "${root}/compile_commands.json"
    ${buildcmd}
}

function tests {
    echo "Testing"
    mkdir -p $builddir
    cd ${builddir}
    ctest --output-on-failure \
          -V -R smf \
          --force-new-ctest-process \
          --schedule-random \
          -j$(nproc) "$@"
}
function release {
    echo "Release"
    builddir=$root/build/release
    mkdir -p $builddir
    cd ${builddir}
    local travis=""
    if [[ "${TRAVIS}" == "1" ]]; then
        echo "Travis build: reducing compilation to only -O1 for speed"
        travis=(-DCMAKE_CXX_FLAGS_RELEASE="-O1 -DNDEBUG"
                -DSMF_ENABLE_BENCHMARK_TESTS=OFF)
    else
        travis=(-DSMF_ENABLE_BENCHMARK_TESTS=ON
                -DSEASTAR_ENABLE_DPDK=ON)
    fi
    cmake     -Wno-dev \
              -DCMAKE_VERBOSE_MAKEFILE=ON \
              -GNinja \
              -DCMAKE_INSTALL_PREFIX=${builddir} \
              -DSMF_ENABLE_CMAKE_PROJECT_FLAGS=ON \
              -DCMAKE_BUILD_TYPE=Release \
              "${travis}" ${root}

    # for fmt.py
    ln -sfn "${builddir}/compile_commands.json" "${root}/compile_commands.json"
    ${buildcmd}
}

function format {
    echo "Format"
    ${root}/tools/fmt.py
}

function package {
    echo "Package"
    mkdir -p $builddir
    cd ${builddir}
    cpack -D CPACK_RPM_PACKAGE_DEBUG=1 \
          -D CPACK_RPM_SPEC_INSTALL_POST="/bin/true" -G RPM;
    cpack -D CPACK_DEBIAN_PACKAGE_DEBUG=1  -G DEB;
}



function usage {
    cat <<EOM
Usage: $(basename "$0") [OPTION]...
  -d          debug build

  -r          release build

  -t          run tests

  -f          format code

  -p          package code

  -b          build bazel

  -h          display help
EOM

    exit 1
}


while getopts ":drtfpb" optKey; do
    case $optKey in
        d)
            debug
            ;;
        r)
            release
            ;;
        t)
            tests
            ;;
        f)
            format
            ;;
        p)
            package
            ;;
        b)
            ${this_dir}/bazel.sh -a
            ;;

        *)
            usage
            ;;
    esac
done

