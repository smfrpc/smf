#!/bin/bash

# Copyright 2019 SMF Authors
#

set -e
set -x

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
proj_dir=$(realpath "${this_dir}"/../)

base_img=${1:-fedora:latest}

# don't attach stdin/tty in CI environment
if [ "${CI}" == "true" ]; then
  extra=""
else
  extra="-it"
fi

docker run --rm --privileged \
       -v "${proj_dir}":/src/smf:z,ro ${extra} \
       -e BUILD_GENERATOR -e CI="${CI}" \
       -e CC="${CC}" -e CXX="${CXX}" \
       -e DEBIAN_FRONTEND=noninteractive \
       -e TZ=$(cat /etc/timezone) \
       -w /src/smf "${base_img}" \
       /bin/bash -c "./install-deps.sh && ci/test.sh"
