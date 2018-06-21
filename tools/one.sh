#!/bin/bash

# Copyright 2018 SMF Authors
#

set -e
set -x
export DEBIAN_FRONTEND=noninteractive
THIS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

name="smf/ci_base_${IMAGE/:/_}"
docker build -t ${name} --build-arg BASE=${IMAGE} -f ${THIS_DIR}/base/Dockerfile .
docker run --rm -e USE_NINJA ${name} /src/smf/tools/run.sh
