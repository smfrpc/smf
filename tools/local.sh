#!/bin/bash

# Copyright 2018 SMF Authors
#

set -e
set -x

THIS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

IMAGES="
fedora:27
"

for img in ${IMAGES}; do
  if [[ ${img} != -* ]]; then
    IMAGE=${img} ${THIS_DIR}/one.sh
  fi
done
