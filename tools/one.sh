#!/bin/bash

# Copyright 2018 SMF Authors
#

set -e
set -x
export DEBIAN_FRONTEND=noninteractive
THIS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT=$(git rev-parse --show-toplevel)
# Always has to be launched from root
# Docker depends on it
cd "$ROOT"
name="smf/ci_base_${IMAGE/:/_}"
docker build -t "${name}" --build-arg BASE="${IMAGE}" -f "${THIS_DIR}"/base/Dockerfile .
# -r (debug) | -t (tests)
docker run --privileged -w /smf --rm "${name}"  ./tools/build.sh -rt
