#!/bin/bash
set -e
set -x

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
proj_dir=${this_dir}/../

base_img=${1:-fedora:latest}

docker run --rm -v ${proj_dir}:/src/smf:z,ro \
  -w /src/smf ${base_img} /bin/bash -c "./install-deps.sh && ci/test.sh"
