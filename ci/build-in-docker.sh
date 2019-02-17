#!/bin/bash
set -e
set -x

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
proj_dir=${this_dir}/../

base_img=${1:-fedora:latest}

# don't attach stdin/tty in CI environment
if [ "${CI}" == "true" ]; then
  extra=""
else
  extra="-it"
fi

# TODO: run as non-root
docker run --rm -v ${proj_dir}:/src/smf:z,ro ${extra} \
  -w /src/smf ${base_img} /bin/bash -c "./install-deps.sh && ci/test.sh"
