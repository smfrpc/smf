#!/bin/bash

# Copyright 2018 SMF Authors
#


set -e


if [[ ! -z ${CI} ]]; then
    echo "In continous integration system..."
    set -x
fi

. /etc/os-release

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd ${this_dir}/.. && pwd)"
buildtype="debug"
builddir=$root/build
function buildcmd() {
    ninja -C $builddir
}
case $ID in
    debian|ubuntu|linuxmint)
        echo "$ID supported"
        ;;
    centos|fedora)
        echo "$ID supported"
        ;;
    *)
        echo "$ID not supported"
        exit 1
        ;;
esac

function debug {
    echo "Debug"
    cd $root
    builddir="$builddir/debug"
    mkdir -p $builddir
    $root/cooking.sh -r wellknown -d $builddir
    # for fmt.py
    ln -sfn "${builddir}/compile_commands.json" "${root}/compile_commands.json"
    buildcmd
}

function tests {
    echo "Testing"
    mkdir -p $builddir
    cd ${builddir}
    ctest --output-on-failure -V -R smf
}
function release {
    echo "Release"
    cd $root
    builddir="$builddir/release"
    mkdir -p $builddir
    local travis=""
    if [[ ! -z ${TRAVIS} ]]; then
        set -x
        echo "Travis build. See raw_logs for output"
        # skip dpdk
        # use -O1 for speed
        $this_dir/travis_stdout.sh \
            "$root/cooking.sh -r wellknown -t Release -d $builddir -- -DCMAKE_CXX_FLAGS_RELEASE='-O1 -DNDEBUG' > $root/travis_cmake.log"
        tail --lines=1000 $root/travis_cmake.log
    else
        $root/cooking.sh -r wellknown -t Release -d $builddir -- -DSMF_ENABLE_BENCHMARK_TESTS=ON
    fi
    # for fmt.py
    ln -sfn "${builddir}/compile_commands.json" "${root}/compile_commands.json"
    buildcmd
}

function format {
    echo "Format"
    cd $root
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
        *)
            usage
            ;;
    esac
done

