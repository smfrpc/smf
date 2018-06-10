#!/bin/bash

# Copyright 2018 SMF Authors
#

set -evx

THIS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR=${THIS_DIR}/../

BUILD_DIR=$(mktemp -d)
INSTALL_DIR=$(mktemp -d)
trap "rm -rf ${BUILD_DIR} ${INSTALL_DIR}" EXIT

WITH_CCACHE=OFF
export CCACHE_DIR=/opt/ccache
if [ -d "${CCACHE_DIR}" ]; then
  WITH_CCACHE=ON
fi

CMAKE_BUILD_TYPE=Release
if [ -n "${USE_NINJA}" ]; then
  CMAKE_GENERATOR="Ninja"
  MAKE=ninja
  MAKE_J=
else
  CMAKE_GENERATOR="Unix Makefiles"
  MAKE=make
  MAKE_J="-j$(nproc)"
fi

pushd ${BUILD_DIR}
cmake -G "${CMAKE_GENERATOR}" \
  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
  -DSMF_ENABLE_INTEGRATION_TESTS=ON \
  -DSMF_ENABLE_UNIT_TESTS=ON \
  -DSMF_ENABLE_BENCHMARK_TESTS=ON \
  -DWITH_CCACHE=${WITH_CCACHE} \
  ${ROOT_DIR}

${MAKE} doc 2> /dev/null
${MAKE} ${MAKE_J}
ctest -V -N -R "^smf"
${MAKE} install
