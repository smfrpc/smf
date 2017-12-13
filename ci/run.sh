#!/bin/bash
set -e
set -x

THIS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR=${THIS_DIR}/../

BUILD_DIR=$(mktemp -d)
INSTALL_DIR=$(mktemp -d)
TEST_DIR=$(mktemp -d)
trap "rm -rf ${BUILD_DIR} ${INSTALL_DIR} ${TEST_DIR}" EXIT

if [ -n "${USE_CLANG}" ]; then
  export CC=clang
  export CXX=clang++
fi

if [ -n "${USE_NINJA}" ]; then
  CMAKE_GENERATOR="Ninja"
  MAKE=ninja
  MAKE_J=
else
  CMAKE_GENERATOR="Unix Makefiles"
  MAKE=make
  MAKE_J="-j$(nproc)"
fi

# configure
pushd ${BUILD_DIR}
cmake -G "${CMAKE_GENERATOR}" \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
  ${ROOT_DIR}

# build docs
${MAKE} doc 2> /dev/null

# build project
${MAKE} ${MAKE_J}
${MAKE} install
echo "Installation size: `du -sh ${INSTALL_DIR}`"


export PATH=${INSTALL_DIR}/bin:${PATH}

# run the unit and integration tests on the installed binaries to catch any
# linking issues.

# integration tests
it_names=(
wal_writer
histograms
rpc
rpc_recv_timeout
wal
clock_pro
cr
)

it_dirs=(
wal_writer
histograms
rpc
rpc_recv_timeout
wal
wal_clock_pro_cache
chain_replication_utils
)

for idx in "${!it_names[@]}"; do
  mkdir ${TEST_DIR}/it_${idx}
  pushd ${TEST_DIR}/it_${idx}

  bname=${it_names[$idx]}_integration_test
  dname=${ROOT_DIR}/src/integration_tests/${it_dirs[$idx]}
  python ${ROOT_DIR}/src/test_runner.py \
    --type integration \
    --binary ${bname} \
    --directory ${dname} \
    --git_root ${ROOT_DIR}

  popd
done

# unit tests
ut_names=(
wal_epoch
file_size_utils
human_output
wal_functor
clockpro
compressors
)

ut_dirs=(
wal_epoch_extractor
file_size_utils
human_bytes_printing_utils
wal_head_file_functor_tests
clock_pro
compressor
)

for idx in "${!ut_names[@]}"; do
  mkdir ${TEST_DIR}/ut_${idx}
  pushd ${TEST_DIR}/ut_${idx}

  bname=${ut_names[$idx]}_unit_test
  dname=${ROOT_DIR}/src/test/${ut_dirs[$idx]}
  python ${ROOT_DIR}/src/test_runner.py \
    --type unit \
    --binary ${bname} \
    --directory ${dname} \
    --git_root ${ROOT_DIR}

  popd
done
