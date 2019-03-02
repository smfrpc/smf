#!/bin/bash

# Copyright 2018 SMF Authors
#

set -e
set -x

THIS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT=$(git rev-parse --show-toplevel)
IMAGES="
ubuntu:bionic
"
# Always has to be launched from root
# Docker depends on it
cd "$ROOT"
for img in ${IMAGES}; do
  if [[ ${img} != -* ]]; then
    IMAGE=${img} "${THIS_DIR}"/one.sh
  fi
done
