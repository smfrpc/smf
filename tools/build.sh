#!/bin/bash
# Copyright 2018 SMF Authors
#

set -e
set -x
if [[ -n ${CI} ]]; then
  echo "In continous integration system..."
  # disable colors so we can parse travis better
  export GCC_COLORS=""
  set -x
fi

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
rootdir="$(cd "${this_dir}"/.. && pwd)"

build_rootdir=${rootdir}/build
builddir=""

# shellcheck disable=SC1091
source /etc/os-release

function buildcmd() {
  ninja -v -C "${1}"
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
    builddir="${build_rootdir}/debug"

    mkdir -p "$builddir"
    cd "${rootdir}"
    cmake -B"${builddir}" -GNinja \
          -DSMF_MANAGE_DEPS=ON \
          -DCMAKE_BUILD_TYPE=Debug -H"${rootdir}"

    # for fmt.py
    ln -sfn "${builddir}/compile_commands.json" "${rootdir}/compile_commands.json"
    buildcmd "${builddir}"
}

function tests {
    echo "Testing"
    cd "${builddir}"
    ctest --output-on-failure -V -R smf
}

function release {
    echo "Release"
    builddir="${build_rootdir}/release"

    mkdir -p "$builddir"
    cd "${rootdir}"
    cmake -B"${builddir}" -GNinja \
          -DSMF_MANAGE_DEPS=ON \
          -DSMF_ENABLE_BENCHMARK_TESTS=ON \
          -DCMAKE_BUILD_TYPE=Release -H"${rootdir}"

    # for fmt.py
    ln -sfn "${builddir}/compile_commands.json" "${rootdir}/compile_commands.json"
    buildcmd "${builddir}"
}

function format {
    echo "Format"
    cd "${rootdir}"
    "${rootdir}"/tools/fmt.py
}

function usage {
    cat <<EOM
Usage: $(basename "$0") [OPTION]...
  -d          debug build

  -r          release build

  -t          run tests

  -f          format code

  -h          display help
EOM

    exit 1
}

# run these after builds
do_tests=""
do_format=""

while getopts ":drtfpb" optKey; do
    case $optKey in
        d)
            debug
            ;;
        r)
            release
            ;;
        t)
            do_tests=true
            ;;
        f)
            do_format=true
            ;;
        *)
            usage
            ;;
    esac
done

if [ "${do_tests}" = "true" ]; then
  tests
fi

if [ "${do_format}" = "true" ]; then
  format
fi

if [ "${do_package}" = "true" ]; then
  package
fi
